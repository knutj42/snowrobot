//#define USE_EXAMPLE_CODE 1

#ifdef USE_EXAMPLE_CODE


/* GStreamer
 * Copyright (C) 2013 Collabora Ltd.
 *   @author Torrie Fischer <torrie.fischer@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

/*
 * An RTP server
 *  creates two sessions and streams audio on one, video on the other, with RTCP
 *  on both sessions. The destination is 127.0.0.1.
 *
 *  In both sessions, we set "rtprtxsend" as the session's "aux" element
 *  in rtpbin, which enables RFC4588 retransmission for that session.
 *
 *  .-------.    .-------.    .-------.      .------------.       .-------.
 *  |audiots|    |alawenc|    |pcmapay|      | rtpbin     |       |udpsink|
 *  |      src->sink    src->sink    src->send_rtp_0 send_rtp_0->sink     |
 *  '-------'    '-------'    '-------'      |            |       '-------'
 *                                           |            |     
 *  .-------.    .---------.    .---------.  |            |       .-------.
 *  |audiots|    |theoraenc|    |theorapay|  |            |       |udpsink|
 *  |      src->sink      src->sink  src->send_rtp_1 send_rtp_1->sink     |
 *  '-------'    '---------'    '---------'  |            |       '-------'
 *                                           |            |
 *                               .------.    |            |
 *                               |udpsrc|    |            |       .-------.
 *                               |     src->recv_rtcp_0   |       |udpsink|
 *                               '------'    |       send_rtcp_0->sink    |
 *                                           |            |       '-------'
 *                               .------.    |            |
 *                               |udpsrc|    |            |       .-------.
 *                               |     src->recv_rtcp_1   |       |udpsink|
 *                               '------'    |       send_rtcp_1->sink    |
 *                                           '------------'       '-------'
 *
 * To keep the set of ports consistent across both this server and the
 * corresponding client, a SessionData struct maps a rtpbin session number to
 * a GstBin and is used to create the corresponding udp sinks with correct
 * ports.
 */

typedef struct _SessionData
{
  int ref;
  guint sessionNum;
  GstElement *input;
} SessionData;

static SessionData *
session_ref (SessionData * data)
{
  g_atomic_int_inc (&data->ref);
  return data;
}

static void
session_unref (gpointer data)
{
  SessionData *session = (SessionData *) data;
  if (g_atomic_int_dec_and_test (&session->ref)) {
    g_free (session);
  }
}

static SessionData *
session_new (guint sessionNum)
{
  SessionData *ret = g_new0 (SessionData, 1);
  ret->sessionNum = sessionNum;
  return session_ref (ret);
}

/*
 * Used to generate informative messages during pipeline startup
 */
static void
cb_state (GstBus * bus, GstMessage * message, gpointer data)
{
  GstObject *pipe = GST_OBJECT (data);
  GstState old, new_state, pending;
  gst_message_parse_state_changed (message, &old, &new_state, &pending);
  if (message->src == pipe) {
    g_print ("Pipeline %s changed state from %s to %s\n",
        GST_OBJECT_NAME (message->src),
        gst_element_state_get_name (old), gst_element_state_get_name (new_state));
  }
}

/*
 * Creates a GstGhostPad named "src" on the given bin, pointed at the "src" pad
 * of the given element
 */
static void
setup_ghost (GstElement * src, GstBin * bin)
{
  GstPad *srcPad = gst_element_get_static_pad (src, "src");
  GstPad *binPad = gst_ghost_pad_new ("src", srcPad);
  gst_element_add_pad (GST_ELEMENT (bin), binPad);
}

static SessionData *
make_audio_session (guint sessionNum)
{
  SessionData *session;
  GstBin *audioBin = GST_BIN (gst_bin_new (NULL));
  GstElement *audioSrc = gst_element_factory_make ("audiotestsrc", NULL);
  GstElement *encoder = gst_element_factory_make ("alawenc", NULL);
  GstElement *payloader = gst_element_factory_make ("rtppcmapay", NULL);
  g_object_set (audioSrc, "is-live", TRUE, NULL);

  gst_bin_add_many (audioBin, audioSrc, encoder, payloader, NULL);
  gst_element_link_many (audioSrc, encoder, payloader, NULL);

  setup_ghost (payloader, audioBin);

  session = session_new (sessionNum);
  session->input = GST_ELEMENT (audioBin);

  return session;
}

static SessionData *
make_video_session (guint sessionNum)
{
  GstBin *videoBin = GST_BIN (gst_bin_new (NULL));
  GstElement *videoSrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *encoder = gst_element_factory_make ("theoraenc", NULL);
  GstElement *payloader = gst_element_factory_make ("rtptheorapay", NULL);
  GstCaps *videoCaps;
  SessionData *session;
  g_object_set (videoSrc, "is-live", TRUE, "horizontal-speed", 1, NULL);
  g_object_set (payloader, "config-interval", 2, NULL);

  gst_bin_add_many (videoBin, videoSrc, encoder, payloader, NULL);
  videoCaps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, 352,
      "height", G_TYPE_INT, 288, "framerate", GST_TYPE_FRACTION, 15, 1, NULL);
  gst_element_link_filtered (videoSrc, encoder, videoCaps);
  gst_element_link (encoder, payloader);

  setup_ghost (payloader, videoBin);

  session = session_new (sessionNum);
  session->input = GST_ELEMENT (videoBin);

  return session;
}

static GstElement *
request_aux_sender (GstElement * rtpbin, guint sessid, SessionData * session)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO ("creating AUX sender");
  bin = gst_bin_new (NULL);
  rtx = gst_element_factory_make ("rtprtxsend", NULL);
  pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "8", G_TYPE_UINT, 98, "96", G_TYPE_UINT, 99, NULL);
  g_object_set (rtx, "payload-type-map", pt_map, NULL);
  gst_structure_free (pt_map);
  gst_bin_add (GST_BIN (bin), rtx);

  pad = gst_element_get_static_pad (rtx, "src");
  name = g_strdup_printf ("src_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (rtx, "sink");
  name = g_strdup_printf ("sink_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  return bin;
}

/*
 * This function sets up the UDP sinks and sources for RTP/RTCP, adds the
 * given session's bin into the pipeline, and links it to the properly numbered
 * pads on the rtpbin
 */
static void
add_stream (GstPipeline * pipe, GstElement * rtpBin, SessionData * session)
{
  GstElement *rtpSink = gst_element_factory_make ("udpsink", NULL);
  GstElement *rtcpSink = gst_element_factory_make ("udpsink", NULL);
  GstElement *rtcpSrc = gst_element_factory_make ("udpsrc", NULL);
  GstElement *identity = gst_element_factory_make ("identity", NULL);
  int basePort;
  gchar *padName;

  basePort = 5000 + (session->sessionNum * 6);

  gst_bin_add_many (GST_BIN (pipe), rtpSink, rtcpSink, rtcpSrc, identity,
      session->input, NULL);

  /* enable retransmission by setting rtprtxsend as the "aux" element of rtpbin */
  g_signal_connect (rtpBin, "request-aux-sender",
      (GCallback) request_aux_sender, session);

  g_object_set (rtpSink, "port", basePort, "host", "127.0.0.1", NULL);
  g_object_set (rtcpSink, "port", basePort + 1, "host", "127.0.0.1", "sync",
      FALSE, "async", FALSE, NULL);
  g_object_set (rtcpSrc, "port", basePort + 5, NULL);

  /* this is just to drop some rtp packets at random, to demonstrate
   * that rtprtxsend actually works */
  g_object_set (identity, "drop-probability", 0.01, NULL);

  padName = g_strdup_printf ("send_rtp_sink_%u", session->sessionNum);
  gst_element_link_pads (session->input, "src", rtpBin, padName);
  g_free (padName);

  /* link rtpbin to udpsink directly here if you don't want
   * artificial packet loss */
  padName = g_strdup_printf ("send_rtp_src_%u", session->sessionNum);
  gst_element_link_pads (rtpBin, padName, identity, "sink");
  gst_element_link (identity, rtpSink);
  g_free (padName);

  padName = g_strdup_printf ("send_rtcp_src_%u", session->sessionNum);
  gst_element_link_pads (rtpBin, padName, rtcpSink, "sink");
  g_free (padName);

  padName = g_strdup_printf ("recv_rtcp_sink_%u", session->sessionNum);
  gst_element_link_pads (rtcpSrc, "src", rtpBin, padName);
  g_free (padName);

  g_print ("New RTP stream on %i/%i/%i\n", basePort, basePort + 1,
      basePort + 5);

  session_unref (session);
}

int
main (int argc, char **argv)
{
  GstPipeline *pipe;
  GstBus *bus;
  SessionData *videoSession;
  SessionData *audioSession;
  GstElement *rtpBin;
  GMainLoop *loop;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  pipe = GST_PIPELINE (gst_pipeline_new (NULL));
  bus = gst_element_get_bus (GST_ELEMENT (pipe));
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (cb_state), pipe);
  gst_bus_add_signal_watch (bus);
  gst_object_unref (bus);

  rtpBin = gst_element_factory_make ("rtpbin", NULL);
  g_object_set (rtpBin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  gst_bin_add (GST_BIN (pipe), rtpBin);

  videoSession = make_video_session (0);
  audioSession = make_audio_session (1);
  add_stream (pipe, rtpBin, videoSession);
  add_stream (pipe, rtpBin, audioSession);

  g_print ("starting server pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_PLAYING);

  g_main_loop_run (loop);

  g_print ("stopping server pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_NULL);

  gst_object_unref (pipe);
  g_main_loop_unref (loop);

  return 0;
}

#else

#include "../common/linebasedserver.h"
#include "../common/gst_wrappers.h"

#include <future>

#include <chrono>
#include <iostream>
#include <typeinfo>

#include <winsock2.h>
#include <boost/program_options.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/parse.hpp>



namespace snowrobot {


  

class CameraInfo {
  public:
    CameraInfo() {
      BOOST_LOG_TRIVIAL(info) << "CameraInfo::CameraInfo() running";

    }

    // This method is called just after a new CameraInfo instance is created.
    // It returns the message that should be sent to the client to inform it of this camera and
    // which network ports the client need to connect to.
    boost::json::object initialize(GstBin* pipeline,
                                   GstElement* rtpbin,
                                   GstDevice * camera_device,
                                   int camera_index) {

      this->pipeline_ = pipeline;
      this->rtpbin_ = rtpbin;
      this->camera_index_ = camera_index;
      BOOST_LOG_TRIVIAL(info) << "CameraInfo::initialize(): creating ksvideosrc";
      GstElement* video_source = gst_element_factory_make(
#ifdef __linux__ 
        "v4l2src",
#else
        "ksvideosrc",
        //"videotestsrc"
#endif
        NULL);
      ASSERT_NOT_NULL(video_source);

      BOOST_LOG_TRIVIAL(info) << "CameraInfo::initialize(): creating x264enc";
      GstElement* x264enc = gst_element_factory_make("x264enc", NULL);
      ASSERT_NOT_NULL(x264enc);
      g_object_set(x264enc, "tune", 4 /*GstX264EncTune  zerolatency (0x00000004) – Zero latency*/   , NULL);
      g_object_set(x264enc, "byte-stream", TRUE, NULL);
      g_object_set(x264enc, "bitrate", 300, NULL);

      BOOST_LOG_TRIVIAL(info) << "CameraInfo::initialize(): creating rtph264pay";
      GstElement* rtph264pay = gst_element_factory_make("rtph264pay", NULL);
      ASSERT_NOT_NULL(rtph264pay);

      BOOST_LOG_TRIVIAL(info) << "CameraInfo::initialize(): creating udpsrc";
      GstElement* video_rtcp_udpsrc = gst_element_factory_make("udpsrc", "video_rtcp_udpsrc");
      g_object_set(video_rtcp_udpsrc, "port", 0, NULL);
      gst_element_set_state(video_rtcp_udpsrc, GST_STATE_PAUSED);
      gint  video_rtcp_udpsrc_assigned_port;
      g_object_get(video_rtcp_udpsrc, "port", &video_rtcp_udpsrc_assigned_port, NULL);
      BOOST_LOG_TRIVIAL(info) << "CameraInfo::initialize(): video_rtcp_udpsrc_assigned_port " << video_rtcp_udpsrc_assigned_port;


      GstElement* queue = gst_element_factory_make("queue", NULL);
      ASSERT_NOT_NULL(queue);

      GstElement* videorate = gst_element_factory_make("videorate", NULL);
      ASSERT_NOT_NULL(videorate);

      GstElement* videoconvert = gst_element_factory_make("videoconvert", NULL);
      ASSERT_NOT_NULL(videoconvert);

      GstCaps* video_caps = gst_caps_from_string("video/x-raw, format=YUY2, width=640, height=360, framerate=30/1, pixel-aspect-ratio=1/1");
      //g_object_set (video_source, "caps", gst_caps_from_string("video/x-raw,width=640,height=360,framerate=15/1"), NULL);
      //g_object_set (videoconvert, "caps", gst_caps_from_string("video/x-raw,width=640,height=360,framerate=15/1"), NULL);


      ASSERT_TRUE(gst_bin_add(pipeline, video_source));
      ASSERT_TRUE(gst_bin_add(pipeline, queue));
      ASSERT_TRUE(gst_bin_add(pipeline, videorate));
      ASSERT_TRUE(gst_bin_add(pipeline, videoconvert));
      ASSERT_TRUE(gst_bin_add(pipeline, x264enc));
      ASSERT_TRUE(gst_bin_add(pipeline, rtph264pay));
      ASSERT_TRUE(gst_bin_add(pipeline, video_rtcp_udpsrc));
      
      //ASSERT_TRUE(gst_element_set_state(pipeline, GST_STATE_READY));


      ASSERT_TRUE(gst_element_link(video_source, queue));
      ASSERT_TRUE(gst_element_link(queue, videorate));
      ASSERT_TRUE(gst_element_link_filtered(videorate, videoconvert, video_caps));
      ASSERT_TRUE(gst_element_link(videoconvert, x264enc));
      ASSERT_TRUE(gst_element_link(x264enc, rtph264pay));

      std::string recv_rtcp_sink_pad_name = "recv_rtcp_sink_" + std::to_string(this->camera_index_);
      ASSERT_TRUE(gst_element_link_pads(video_rtcp_udpsrc, "src", rtpbin, recv_rtcp_sink_pad_name.c_str()));

      std::string send_rtp_sink_pad_name = "send_rtp_sink_" + std::to_string(this->camera_index_);
      ASSERT_TRUE(gst_element_link_pads(rtph264pay, "src", rtpbin, send_rtp_sink_pad_name.c_str()));

      boost::json::object camera;
      std::string display_name = string_from_gchar(gst_device_get_display_name(camera_device));
      camera["name"] = display_name;
      camera["video_rtcp_udpsrc_port"] = video_rtcp_udpsrc_assigned_port;

      auto device_gst_caps = make_GstCaps_ptr(gst_device_get_caps(camera_device));
      std::string device_gst_caps_str = string_from_gchar(gst_caps_to_string(device_gst_caps.get()));
      boost::json::array resolutions;
      auto for_each_caps2 = [] (GstCapsFeatures * features,
                        GstStructure * structure,
                        gpointer user_data) -> gboolean {
        std::string structure_str = string_from_gchar(gst_structure_to_string(structure));
        boost::json::array* resolutions = (boost::json::array*)user_data;
        resolutions->emplace_back(structure_str);
        return TRUE;
      };
      gst_caps_foreach(device_gst_caps.get(), for_each_caps2, &resolutions);
      camera["resolutions"] = std::move(resolutions);
      return std::move(camera);
    }

    // This method is called when the client has sent a info-message about the desired resolution, udp ports, etc.
    void setClientInfo(boost::asio::ip::tcp::socket& client_socket, const boost::json::object& client_info) {
      gint video_client_rtcp_udpsrc_port = client_info.at("video_rtcp_udpsrc_port").as_int64();
      gint video_client_rtp_udpsrc_port = client_info.at("video_rtp_udpsrc_port").as_int64();

      GstElement* video_rtcp_udpsink = gst_element_factory_make("udpsink", "video_rtcp_udpsink");
      GstElement* video_rtp_udpsink = gst_element_factory_make("udpsink", "video_rtp_udpsink");

      std::string client_address = client_socket.remote_endpoint().address().to_string();
      g_object_set(video_rtcp_udpsink, "port", video_client_rtcp_udpsrc_port, NULL);
      g_object_set(video_rtcp_udpsink, "host", client_address.c_str(), NULL);
      g_object_set(video_rtcp_udpsink, "sync", FALSE, NULL);
      g_object_set(video_rtcp_udpsink, "async", FALSE, NULL);

      g_object_set(video_rtp_udpsink, "port", video_client_rtp_udpsrc_port, NULL);
      g_object_set(video_rtp_udpsink, "host", client_address.c_str(), NULL);
      g_object_set(video_rtp_udpsink, "ts-offset", 0, NULL);

      ASSERT_TRUE(gst_bin_add(this->pipeline_, video_rtcp_udpsink));
      ASSERT_TRUE(gst_bin_add(this->pipeline_, video_rtp_udpsink));

      std::string send_rtcp_src_pad_name = "send_rtcp_src_" + std::to_string(this->camera_index_);
      ASSERT_TRUE(gst_element_link_pads(this->rtpbin_, send_rtcp_src_pad_name.c_str(), video_rtcp_udpsink, "sink"));

      std::string send_rtp_src_pad_name = "send_rtp_src_" + std::to_string(this->camera_index_);
      ASSERT_TRUE(gst_element_link_pads(this->rtpbin_, send_rtp_src_pad_name.c_str(), video_rtp_udpsink, "sink"));

    }
  private:
    GstBin* pipeline_;
    GstElement* rtpbin_;
    int camera_index_;
};



static void
cb_eos (GstBus * bus, GstMessage * message, gpointer data)
{
  g_print ("Got EOS\n");
  //g_main_loop_quit (loop);
}

static void
cb_state (GstBus * bus, GstMessage * message, gpointer data)
{
  GstObject *pipe = GST_OBJECT (data);
  GstState old, new_state, pending;
  gst_message_parse_state_changed (message, &old, &new_state, &pending);
  if (message->src == pipe) {
    g_print ("Pipeline %s changed state from %s to %s\n",
        GST_OBJECT_NAME (message->src),
        gst_element_state_get_name (old), gst_element_state_get_name (new_state));
  }
}

static void
cb_warning (GstBus * bus, GstMessage * message, gpointer data)
{
  GError *error = NULL;
  gst_message_parse_warning (message, &error, NULL);
  g_printerr ("Got warning from %s: %s\n", GST_OBJECT_NAME (message->src),
      error->message);
  g_error_free (error);
}

static void
cb_error (GstBus * bus, GstMessage * message, gpointer data)
{
  GError *error = NULL;
  gst_message_parse_error (message, &error, NULL);
  //g_printerr ("Got error from %s: %s\n", GST_OBJECT_NAME (message->src),
  //    error->message);
  BOOST_LOG_TRIVIAL(error) << "cb_error:" << error->message; 
  g_error_free (error);
  //g_main_loop_quit (loop);
}



int main(int argc, char** argv)
{
  gst_init(NULL, NULL);
  const gchar *nano_str;
  guint major, minor, micro, nano;
  gst_version (&major, &minor, &micro, &nano);
  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";

  BOOST_LOG_TRIVIAL(info) << "This program is linked against GStreamer " << major << "." << minor << "." << micro << " " << nano_str;
  
  GList_ptr devices;

  // At least on Windows11, the gst_device_monitor_get_devices() function sometimes doesn't find the integrated camera on my laptop.
  // It usually works to call the function multiple times until the camera is found. I have no clue what causes this behaviour, but
  // the retries seems to work ok for now. 
  size_t camera_count = 0;
  size_t attempt_nr = 0;
  constexpr size_t max_attempts = 10;
  while(true) {
    attempt_nr += 1;
    BOOST_LOG_TRIVIAL(info) << "Getting the video and audio devices (attempt " << attempt_nr << "/" << max_attempts << "). This can take a minute or two...";
    auto monitor = make_GstDeviceMonitor_ptr(gst_device_monitor_new());
    if(!gst_device_monitor_start(monitor.get())) {
      BOOST_LOG_TRIVIAL(error) << "GstDeviceMonitor couldn't started!";
      exit(-1);
    }

    devices = make_GList_ptr(gst_device_monitor_get_devices(monitor.get()));

    for (GList* devIter = g_list_first(devices.get()); devIter != nullptr; devIter=g_list_next(devIter)) {
      GstDevice * device = (GstDevice*) devIter->data;
      if (device == nullptr) {
        continue;
      }

      std::string device_class = string_from_gchar(gst_device_get_device_class(device));
      std::string display_name = string_from_gchar(gst_device_get_display_name(device));

      if (device_class != "Audio/Source" && device_class != "Video/Source") {
        continue;
      }
      if (device_class == "Video/Source") {
        camera_count += 1;
      }
      BOOST_LOG_TRIVIAL(info) << "device_class: " << device_class << ": " << display_name;
    }
  
    if (camera_count > 0) {
      // We found at least one camera. Good.
      break;
    } else {
      if (attempt_nr >= max_attempts) {
        std::ostringstream msg;
        msg << "Failed to find any cameras after " << attempt_nr << " attempts! Giving up!";
        BOOST_LOG_TRIVIAL(error) << msg.str();
        throw std::runtime_error(msg.str());
      } else {
        BOOST_LOG_TRIVIAL(warning) << "Failed to find any cameras after " << attempt_nr << "/" << max_attempts << " attempts. I'll try again in a few seconds.";
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      }
    }
  }

  int debug_port_nr;
  int command_port_nr;
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
      ("debug-port", boost::program_options::value<int>(&debug_port_nr)->default_value(0), "debug port")
      ("command-port", boost::program_options::value<int>(&command_port_nr)->default_value(20000), "command port")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    

  boost::asio::io_context ctx;


  GstElement* pipeline = NULL;

  GMainLoop* loop = g_main_loop_new(NULL, FALSE);
  ASSERT_NOT_NULL(loop);

  auto loop_runner_func = [&] {
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() starting.";
      g_main_loop_run (loop);
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() finished ok.";
  };

  auto loop_runner_future = std::async(std::launch::async, loop_runner_func);

  // The debug port used for ci-tests and for manual debugging.
  std::unique_ptr<LineBasedServer> debug_port;
  if (debug_port_nr > 0) {
    debug_port = std::make_unique<LineBasedServer>(
      ctx,
      debug_port_nr,
    
    [](boost::asio::ip::tcp::socket& sock) {
      BOOST_LOG_TRIVIAL(info) << "Got a new debug-port connection from '" << sock.remote_endpoint() << "'";
      return std::string("");
    },

    [](boost::asio::ip::tcp::socket& sock) {
      BOOST_LOG_TRIVIAL(info) << "Lost the debug-port connection from '" << sock.remote_endpoint() << "'";
    },

    [](boost::asio::ip::tcp::socket& sock, const std::string& request) {
      BOOST_LOG_TRIVIAL(info) << "Got a debug-port message from '" << sock.remote_endpoint() << "': " << request;
      std::string response;
      if (request == "ping") {
        response = "pong";
      } else {
        response = "ERROR: unknown request '" + request + "'";
      }
      return response;
    });
  }


  GstBusFunc bus_callback = nullptr;

  std::map<std::string, CameraInfo> camera_infos;

  // The command port is where the client application connects to the server.
  bool has_active_client = false;
  boost::asio::ip::tcp::endpoint active_client;
  LineBasedServer command_port(
    ctx,
    command_port_nr,
     
    [&](boost::asio::ip::tcp::socket& sock) -> std::string {
      BOOST_LOG_TRIVIAL(info) << "Got a new connection from '" << sock.remote_endpoint() << "'";
      if (has_active_client) {
        return "There is already an active client";
      }
      has_active_client = true;
      active_client = sock.remote_endpoint();

      BOOST_LOG_TRIVIAL(info) << "Calling gst_pipeline_new()";
      pipeline = gst_pipeline_new(NULL);

      // add a gstreamer message handler      
      /*bus_callback = function_pointer<gboolean(GstBus*, GstMessage*, gpointer)>([] (GstBus*, GstMessage* msg, gpointer) -> gboolean {
        BOOST_LOG_TRIVIAL(info) << "bus_callback() running. msg:" << gst_message_type_get_name(msg->type) << " type:" << msg->type;
        return TRUE;
      });
      GstBus* bus = gst_pipeline_get_bus((GstPipeline*)pipeline);
      guint bus_watch_id = gst_bus_add_watch (bus, bus_callback, loop);
      gst_object_unref (bus);*/

      GstBus* bus = gst_element_get_bus((GstElement*)pipeline);
      g_signal_connect (bus, "message::error", G_CALLBACK (cb_error), pipeline);
      g_signal_connect (bus, "message::warning", G_CALLBACK (cb_warning), pipeline);
      g_signal_connect (bus, "message::state-changed", G_CALLBACK (cb_state), pipeline);
      g_signal_connect (bus, "message::eos", G_CALLBACK (cb_eos), NULL);
      gst_bus_add_signal_watch (bus);

      gst_object_unref (bus);

      GstElement* rtpbin = gst_element_factory_make("rtpbin", NULL);
      gst_bin_add_many(GST_BIN_CAST(pipeline), rtpbin, NULL);
      
      boost::json::array cameras;
      int camere_index = -1;
      for (GList* devIter = g_list_first(devices.get()); devIter != nullptr; devIter=g_list_next(devIter)) {
        GstDevice * device = (GstDevice*) devIter->data;
        if (device == nullptr) {
          continue;
        }

        std::string device_class = string_from_gchar(gst_device_get_device_class(device));
        if (device_class != "Video/Source") {
          continue;
        }
        camere_index++;
        std::string display_name = string_from_gchar(gst_device_get_display_name(device));
        auto find = camera_infos.find(display_name);
        if (find != camera_infos.end()) {
          BOOST_LOG_TRIVIAL(error) << "Got a duplicate camera name '" << display_name << "'!";
          exit(-1);
        }
        CameraInfo& camera_info = camera_infos[display_name];  // This will insert a new CameraInfo entry int the map

        boost::json::object camera = std::move(camera_info.initialize(GST_BIN_CAST(pipeline), rtpbin, device, camere_index));

        cameras.push_back(std::move(camera));
      }

      boost::json::object cameras_msg;
      cameras_msg["type"] = "cameras";
      cameras_msg["cameras"] = std::move(cameras);
      std::string cameras_msg_str = boost::json::serialize(cameras_msg);
      BOOST_LOG_TRIVIAL(info) << "Sending this camera list to '" << sock.remote_endpoint() << "': " << cameras_msg_str;
      return cameras_msg_str;
    },

    [&](boost::asio::ip::tcp::socket& sock) {
      BOOST_LOG_TRIVIAL(info) << "Lost the connection from '" << sock.remote_endpoint() << "'";
      if (sock.remote_endpoint() == active_client) {
        camera_infos.clear();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
        has_active_client = false;
      }
    },

    [&](boost::asio::ip::tcp::socket& sock, const std::string& request) {
      std::string response;
      if (request == "ping") {
        response = "pong";

      } else {
        BOOST_LOG_TRIVIAL(info) << "Got a message from '" << sock.remote_endpoint() << "': " << request;

        boost::json::object request_obj = boost::json::parse(request).as_object();
        std::string request_type(request_obj.at("type").as_string());
        if (request_type == "welcome-response") {
          boost::json::array camera_responses = request_obj.at("cameras").as_array();
          for (boost::json::value& value : camera_responses) {
            boost::json::object& camera_response = value.as_object();
            std::string camera_name(camera_response.at("name").as_string());
            auto camera_info_find = camera_infos.find(camera_name);
            if (camera_info_find == camera_infos.end()) {
              std::ostringstream msg;
              msg << "Unknown camera name: '" << camera_name << "'";
              throw std::runtime_error(msg.str());
            }
            CameraInfo& camera_info = camera_info_find->second;
            camera_info.setClientInfo(sock, camera_response); 
          }

          BOOST_LOG_TRIVIAL(info) << "Calling gst_debug_bin_to_dot_file()";
          GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)pipeline, GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE, "server.dot");
          BOOST_LOG_TRIVIAL(info) << "gst_debug_bin_to_dot_file() finished ok";

          GstStateChangeReturn result = gst_element_set_state(pipeline, GST_STATE_PLAYING);
          BOOST_LOG_TRIVIAL(info) << "Started the gstreamer pipeline. result:" << result;
          if (result == GST_STATE_CHANGE_FAILURE) {
            throw std::runtime_error("Failed to start the gstreamer pipeline!");
          }


          // we don't want to send a response to this message, so we return an empty string.
          response = "";
        } else {
          BOOST_LOG_TRIVIAL(info) << "Got an unknown request type: '" << request_type << "' from '" << sock.remote_endpoint();
          std::ostringstream msg;
          msg << "Unknown request type: '" << request_type << "'";
          throw std::runtime_error(msg.str());
        }

      }
      return response;
    }
  );

  BOOST_LOG_TRIVIAL(info) << "server starting up.";
  auto asio_main_future = std::async(std::launch::async, [&ctx]{ctx.run();});

  BOOST_LOG_TRIVIAL(info) << "Calling asio_main_future.get();";
  asio_main_future.get();

  g_main_loop_quit(loop);
  loop_runner_future.get();

  BOOST_LOG_TRIVIAL(info) << "server shutting down.";
  return 0;
}


}



int main(int argc, char** argv) {
    return snowrobot::main(argc, argv);
}
#endif