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
#include <stdlib.h>

/*
 * RTP receiver with RFC4588 retransmission handling enabled
 *
 *  In this example we have two RTP sessions, one for video and one for audio.
 *  Video is received on port 5000, with its RTCP stream received on port 5001
 *  and sent on port 5005. Audio is received on port 5005, with its RTCP stream
 *  received on port 5006 and sent on port 5011.
 *
 *  In both sessions, we set "rtprtxreceive" as the session's "aux" element
 *  in rtpbin, which enables RFC4588 retransmission handling for that session.
 *
 *             .-------.      .----------.        .-----------.   .---------.   .-------------.
 *  RTP        |udpsrc |      | rtpbin   |        |theoradepay|   |theoradec|   |autovideosink|
 *  port=5000  |      src->recv_rtp_0 recv_rtp_0->sink       src->sink     src->sink          |
 *             '-------'      |          |        '-----------'   '---------'   '-------------'
 *                            |          |
 *                            |          |     .-------.
 *                            |          |     |udpsink|  RTCP
 *                            |  send_rtcp_0->sink     | port=5005
 *             .-------.      |          |     '-------' sync=false
 *  RTCP       |udpsrc |      |          |               async=false
 *  port=5001  |     src->recv_rtcp_0    |
 *             '-------'      |          |
 *                            |          |
 *             .-------.      |          |        .---------.   .-------.   .-------------.
 *  RTP        |udpsrc |      |          |        |pcmadepay|   |alawdec|   |autoaudiosink|
 *  port=5006  |      src->recv_rtp_1 recv_rtp_1->sink     src->sink   src->sink          |
 *             '-------'      |          |        '---------'   '-------'   '-------------'
 *                            |          |
 *                            |          |     .-------.
 *                            |          |     |udpsink|  RTCP
 *                            |  send_rtcp_1->sink     | port=5011
 *             .-------.      |          |     '-------' sync=false
 *  RTCP       |udpsrc |      |          |               async=false
 *  port=5007  |     src->recv_rtcp_1    |
 *             '-------'      '----------'
 *
 */

GMainLoop *loop = NULL;

typedef struct _SessionData
{
  int ref;
  GstElement *rtpbin;
  guint sessionNum;
  GstCaps *caps;
  GstElement *output;
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
    g_object_unref (session->rtpbin);
    gst_caps_unref (session->caps);
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

static void
setup_ghost_sink (GstElement * sink, GstBin * bin)
{
  GstPad *sinkPad = gst_element_get_static_pad (sink, "sink");
  GstPad *binPad = gst_ghost_pad_new ("sink", sinkPad);
  gst_element_add_pad (GST_ELEMENT (bin), binPad);
}

static SessionData *
make_audio_session (guint sessionNum)
{
  SessionData *ret = session_new (sessionNum);
  GstBin *bin = GST_BIN (gst_bin_new ("audio"));
  GstElement *queue = gst_element_factory_make ("queue", NULL);
  GstElement *sink = gst_element_factory_make ("autoaudiosink", NULL);
  GstElement *audioconvert = gst_element_factory_make ("audioconvert", NULL);
  GstElement *audioresample = gst_element_factory_make ("audioresample", NULL);
  GstElement *depayloader = gst_element_factory_make ("rtppcmadepay", NULL);
  GstElement *decoder = gst_element_factory_make ("alawdec", NULL);

  gst_bin_add_many (bin, queue, depayloader, decoder, audioconvert,
      audioresample, sink, NULL);
  gst_element_link_many (queue, depayloader, decoder, audioconvert,
      audioresample, sink, NULL);

  setup_ghost_sink (queue, bin);

  ret->output = GST_ELEMENT (bin);
  ret->caps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "clock-rate", G_TYPE_INT, 8000,
      "encoding-name", G_TYPE_STRING, "PCMA", NULL);

  return ret;
}

static SessionData *
make_video_session (guint sessionNum)
{
  SessionData *ret = session_new (sessionNum);
  GstBin *bin = GST_BIN (gst_bin_new ("video"));
  GstElement *queue = gst_element_factory_make ("queue", NULL);
  GstElement *depayloader = gst_element_factory_make ("rtptheoradepay", NULL);
  GstElement *decoder = gst_element_factory_make ("theoradec", NULL);
  GstElement *converter = gst_element_factory_make ("videoconvert", NULL);
  GstElement *sink = gst_element_factory_make ("autovideosink", NULL);

  gst_bin_add_many (bin, depayloader, decoder, converter, queue, sink, NULL);
  gst_element_link_many (queue, depayloader, decoder, converter, sink, NULL);

  setup_ghost_sink (queue, bin);

  ret->output = GST_ELEMENT (bin);
  ret->caps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "video",
      "clock-rate", G_TYPE_INT, 90000,
      "encoding-name", G_TYPE_STRING, "THEORA", NULL);

  return ret;
}

static GstCaps *
request_pt_map (GstElement * rtpbin, guint session, guint pt,
    gpointer user_data)
{
  SessionData *data = (SessionData *) user_data;
  gchar *caps_str;
  g_print ("Looking for caps for pt %u in session %u, have %u\n", pt, session,
      data->sessionNum);
  if (session == data->sessionNum) {
    caps_str = gst_caps_to_string (data->caps);
    g_print ("Returning %s\n", caps_str);
    g_free (caps_str);
    return gst_caps_ref (data->caps);
  }
  return NULL;
}

static void
cb_eos (GstBus * bus, GstMessage * message, gpointer data)
{
  g_print ("Got EOS\n");
  g_main_loop_quit (loop);
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
  g_printerr ("Got error from %s: %s\n", GST_OBJECT_NAME (message->src),
      error->message);
  g_error_free (error);
  g_main_loop_quit (loop);
}

static void
handle_new_stream (GstElement * element, GstPad * newPad, gpointer data)
{
  SessionData *session = (SessionData *) data;
  gchar *padName;
  gchar *myPrefix;

  padName = gst_pad_get_name (newPad);
  myPrefix = g_strdup_printf ("recv_rtp_src_%u", session->sessionNum);

  g_print ("New pad: %s, looking for %s_*\n", padName, myPrefix);

  if (g_str_has_prefix (padName, myPrefix)) {
    GstPad *outputSinkPad;
    GstElement *parent;

    parent = GST_ELEMENT (gst_element_get_parent (session->rtpbin));
    gst_bin_add (GST_BIN (parent), session->output);
    gst_element_sync_state_with_parent (session->output);
    gst_object_unref (parent);

    outputSinkPad = gst_element_get_static_pad (session->output, "sink");
    g_assert_cmpint (gst_pad_link (newPad, outputSinkPad), ==, GST_PAD_LINK_OK);
    gst_object_unref (outputSinkPad);

    g_print ("Linked!\n");
  }
  g_free (myPrefix);
  g_free (padName);
}

static GstElement *
request_aux_receiver (GstElement * rtpbin, guint sessid, SessionData * session)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO ("creating AUX receiver");
  bin = gst_bin_new (NULL);
  rtx = gst_element_factory_make ("rtprtxreceive", NULL);
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

static void
join_session (GstElement * pipeline, GstElement * rtpBin, SessionData * session)
{
  GstElement *rtpSrc;
  GstElement *rtcpSrc;
  GstElement *rtcpSink;
  gchar *padName;
  guint basePort;

  g_print ("Joining session %p\n", session);

  session->rtpbin = g_object_ref (rtpBin);

  basePort = 5000 + (session->sessionNum * 6);

  rtpSrc = gst_element_factory_make ("udpsrc", NULL);
  rtcpSrc = gst_element_factory_make ("udpsrc", NULL);
  rtcpSink = gst_element_factory_make ("udpsink", NULL);
  g_object_set (rtpSrc, "port", basePort, "caps", session->caps, NULL);
  g_object_set (rtcpSink, "port", basePort + 5, "host", "127.0.0.1", "sync",
      FALSE, "async", FALSE, NULL);
  g_object_set (rtcpSrc, "port", basePort + 1, NULL);

  g_print ("Connecting to %i/%i/%i\n", basePort, basePort + 1, basePort + 5);

  /* enable RFC4588 retransmission handling by setting rtprtxreceive
   * as the "aux" element of rtpbin */
  g_signal_connect (rtpBin, "request-aux-receiver",
      (GCallback) request_aux_receiver, session);

  gst_bin_add_many (GST_BIN (pipeline), rtpSrc, rtcpSrc, rtcpSink, NULL);

  g_signal_connect_data (rtpBin, "pad-added", G_CALLBACK (handle_new_stream),
      session_ref (session), (GClosureNotify) session_unref, (GConnectFlags)0);

  g_signal_connect_data (rtpBin, "request-pt-map", G_CALLBACK (request_pt_map),
      session_ref (session), (GClosureNotify) session_unref, (GConnectFlags)0);

  padName = g_strdup_printf ("recv_rtp_sink_%u", session->sessionNum);
  gst_element_link_pads (rtpSrc, "src", rtpBin, padName);
  g_free (padName);

  padName = g_strdup_printf ("recv_rtcp_sink_%u", session->sessionNum);
  gst_element_link_pads (rtcpSrc, "src", rtpBin, padName);
  g_free (padName);

  padName = g_strdup_printf ("send_rtcp_src_%u", session->sessionNum);
  gst_element_link_pads (rtpBin, padName, rtcpSink, "sink");
  g_free (padName);

  session_unref (session);
}

int
main (int argc, char **argv)
{
  GstPipeline *pipe;
  SessionData *videoSession;
  SessionData *audioSession;
  GstElement *rtpBin;
  GstBus *bus;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);
  pipe = GST_PIPELINE (gst_pipeline_new (NULL));

  bus = gst_element_get_bus (GST_ELEMENT (pipe));
  g_signal_connect (bus, "message::error", G_CALLBACK (cb_error), pipe);
  g_signal_connect (bus, "message::warning", G_CALLBACK (cb_warning), pipe);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (cb_state), pipe);
  g_signal_connect (bus, "message::eos", G_CALLBACK (cb_eos), NULL);
  gst_bus_add_signal_watch (bus);
  gst_object_unref (bus);

  rtpBin = gst_element_factory_make ("rtpbin", NULL);
  gst_bin_add (GST_BIN (pipe), rtpBin);
  g_object_set (rtpBin, "latency", 200, "do-retransmission", TRUE,
      "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  videoSession = make_video_session (0);
  audioSession = make_audio_session (1);

  join_session (GST_ELEMENT (pipe), rtpBin, videoSession);
  join_session (GST_ELEMENT (pipe), rtpBin, audioSession);

  g_print ("starting client pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_PLAYING);

  g_main_loop_run (loop);

  g_print ("stopping client pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_NULL);

  gst_object_unref (pipe);
  g_main_loop_unref (loop);

  return 0;
}


#else

#include "../common/linebasedserver.h"
#include "../common/gst_wrappers.h"

#include <winsock2.h>

#include <cmath>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QTNetwork/QTcpSocket>

#include <array>
#include <list>
#include <string>
#include <chrono>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <future>

#include <gst/gst.h>
#include <gst/rtp/rtp.h>

#include <gst/video/videooverlay.h>

#include <boost/program_options.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/parse.hpp>


namespace snowrobot {


GstElement* global_video_rtp_udpsrc_hack = nullptr;

static void
pad_added_handler(GstElement *element,
        GstPad     *pad,
        gpointer    data)
{
  gchar *name;

  name = gst_pad_get_name (pad);
  BOOST_LOG_TRIVIAL(info) << "A new pad '" << name << "' was created";
 
  /*if (g_str_has_prefix (name, "recv_rtp_src_")) {

    outputSinkPad = gst_element_get_static_pad (session->output, "sink");
    g_assert_cmpint (gst_pad_link (newPad, outputSinkPad), ==, GST_PAD_LINK_OK);
    gst_object_unref (outputSinkPad);

    g_print ("Linked!\n");
  }*/


  g_free (name);

 
  /* here, you would setup a new pad link for the newly created pad */

}

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
  // g_printerr ("Got error from %s: %s\n", GST_OBJECT_NAME (message->src),
    //  error->message);
  
  BOOST_LOG_TRIVIAL(error) << "cb_error:" << error->message; 

  g_error_free (error);
  //g_main_loop_quit (loop);
}


class CameraView : public QFrame {
  public:
    CameraView() : QFrame() {

    }
    


    static void pad_added_handler(GstElement *element, GstPad *pad, gpointer data) {
      std::string pad_name = string_from_gchar(gst_pad_get_name(pad));
      BOOST_LOG_TRIVIAL(info) << "A new pad '" << pad_name << "' was created";
      CameraView* camera_view = (CameraView*)data;
      std::string prefix = "recv_rtp_src_" + std::to_string(camera_view->camera_index_) + "_";
      if (pad_name.find(prefix) == 0) {
        GstPad* sink_pad =gst_element_get_static_pad(camera_view->h264depay_, "sink");
        ASSERT_NOT_NULL(sink_pad);
        if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
          THROW_RUNTIME_ERROR("Failed to link the new pad!");
        }
      }      
    }

    boost::json::object initialize(const std::string& server_host,
               GstBin* pipeline, GstElement* rtpbin,
               const boost::json::object& camera,
               int camera_index
               )  {
      pipeline_ = pipeline;
      rtpbin_ = rtpbin;
      camera_index_ = camera_index;
      std::string camera_name(camera.at("name").as_string());
      int64_t video_rtcp_udpsrc_port = camera.at("video_rtcp_udpsrc_port").as_int64();

      for (const boost::json::value& resolution: camera.at("resolutions").as_array()) {
        std::string resolution_str(resolution.as_string());
        if (resolution_str.find("video") != 0) {
          // this isn't a video-resolution, so skip it. We aren't interested in still-frame resolutions.
          continue;
        }
        resolutions_.push_back(resolution_str);
      }
      
      
      this->camera_name = camera_name;
      QVBoxLayout* layout = new QVBoxLayout();
      this->setLayout(layout);
      resolutions_selector = new QComboBox();
      for(const std::string& resolution: resolutions_) {
        resolutions_selector->addItem(QString::fromStdString(resolution));
      }
      layout->addWidget(resolutions_selector);
      video_widget = new QWidget();
      video_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      video_widget->setMinimumSize(200,200);
      video_widget->setStyleSheet("background-color: pink;");

      layout->addWidget(video_widget);


      auto winId = video_widget->winId();
      BOOST_LOG_TRIVIAL(info) << "CameraView::CameraView(): video_widget->getVideoWidgetWinId() " << winId;

      this->h264depay_ = gst_element_factory_make("rtph264depay", NULL);
      ASSERT_NOT_NULL(this->h264depay_);
      ASSERT_TRUE(gst_bin_add(pipeline, this->h264depay_));

      this->h264dec_ = gst_element_factory_make("avdec_h264", NULL);
      ASSERT_NOT_NULL(this->h264dec_);
      ASSERT_TRUE(gst_bin_add(pipeline, this->h264dec_));

      this->videosink_ = gst_element_factory_make("d3dvideosink", NULL);
      ASSERT_NOT_NULL(this->videosink_);
      ASSERT_TRUE(gst_bin_add(pipeline, this->videosink_));

      video_rtp_udpsrc_ = gst_element_factory_make("udpsrc", "video_rtp_udpsrc_");
      ASSERT_NOT_NULL(video_rtp_udpsrc_);
      // temp hack:
      global_video_rtp_udpsrc_hack = video_rtp_udpsrc_;
      g_object_set(video_rtp_udpsrc_, "port", 0, NULL);
      GstCaps* video_rtp_udpsrc_caps = gst_caps_from_string("application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264");
      g_object_set(video_rtp_udpsrc_, "caps", video_rtp_udpsrc_caps, NULL);

      gst_element_set_state(video_rtp_udpsrc_, GST_STATE_PAUSED);
      
      
      gint  video_rtp_udpsrc_assigned_port;
      g_object_get(video_rtp_udpsrc_, "port", &video_rtp_udpsrc_assigned_port, NULL);
      BOOST_LOG_TRIVIAL(info) << "CameraView::CameraView(): video_rtp_udpsrc_assigned_port " << video_rtp_udpsrc_assigned_port;
      ASSERT_TRUE(gst_bin_add(pipeline, video_rtp_udpsrc_));


      video_rtcp_udpsrc_ = gst_element_factory_make("udpsrc", "video_rtcp_udpsrc_");
      ASSERT_NOT_NULL(video_rtcp_udpsrc_);
      g_object_set(video_rtcp_udpsrc_, "port", 0, NULL );
      gst_element_set_state(video_rtcp_udpsrc_, GST_STATE_PAUSED);
      gint  video_rtcp_udpsrc_assigned_port;
      g_object_get(video_rtcp_udpsrc_, "port", &video_rtcp_udpsrc_assigned_port, NULL);
      BOOST_LOG_TRIVIAL(info) << "CameraView::CameraView(): video_rtcp_udpsrc_assigned_port " << video_rtcp_udpsrc_assigned_port;
      ASSERT_TRUE(gst_bin_add(pipeline, video_rtcp_udpsrc_));


      video_rtcp_udpsink_ = gst_element_factory_make("udpsink", "video_rtcp_udpsink_");
      g_object_set(video_rtcp_udpsink_, "host", server_host, NULL);
      g_object_set(video_rtcp_udpsink_, "port", video_rtcp_udpsrc_port, NULL);
      g_object_set(video_rtcp_udpsink_, "sync", false, NULL );
      g_object_set(video_rtcp_udpsink_, "async", false, NULL );
      ASSERT_TRUE(gst_bin_add(pipeline, video_rtcp_udpsink_));


    
      std::string send_rtcp_src_pad_name = "send_rtcp_src_" + std::to_string(camera_index);
      ASSERT_TRUE(gst_element_link_pads(rtpbin, send_rtcp_src_pad_name.c_str(), video_rtcp_udpsink_, "sink"));

      std::string recv_rtcp_sink_pad_name = "recv_rtcp_sink_" + std::to_string(camera_index);
      ASSERT_TRUE(gst_element_link_pads(video_rtcp_udpsrc_, "src", rtpbin, recv_rtcp_sink_pad_name.c_str()));

      std::string recv_rtp_sink_pad_name = "recv_rtp_sink_" + std::to_string(camera_index);
      ASSERT_TRUE(gst_element_link_pads(video_rtp_udpsrc_, "src", rtpbin, recv_rtp_sink_pad_name.c_str()));

      //std::string recv_rtp_src_pad_name = "recv_rtp_src_" + std::to_string(camera_index);
      //ASSERT_TRUE(gst_element_link_pads(rtpbin, recv_rtp_src_pad_name.c_str(), this->h264depay_, "sink"));

      ASSERT_TRUE(gst_element_link(this->h264depay_, this->h264dec_));
      ASSERT_TRUE(gst_element_link(this->h264dec_, this->videosink_));

      gst_video_overlay_set_window_handle((GstVideoOverlay*)this->videosink_, winId);

      boost::json::object response_msg;
      response_msg["name"] = camera_name;
      response_msg["camera_index"] = camera_index;
      response_msg["video_rtp_udpsrc_port"] = video_rtp_udpsrc_assigned_port;
      response_msg["video_rtcp_udpsrc_port"] = video_rtcp_udpsrc_assigned_port;

      g_signal_connect(rtpbin, "pad-added", G_CALLBACK(CameraView::pad_added_handler), this);

      return std::move(response_msg);
    }

    const std::list<std::string>& getResolutions() const {
      return resolutions_;
    }

    ~CameraView() {
      if (pipeline_) {
        if (video_rtp_udpsrc_) {
          gst_bin_remove(pipeline_, video_rtp_udpsrc_);
        }

        if (video_rtcp_udpsrc_) {
          gst_bin_remove(pipeline_, video_rtcp_udpsrc_);
        }

        if (video_rtcp_udpsink_) {
          gst_bin_remove(pipeline_, video_rtcp_udpsink_);
        }

        if (h264depay_) {
          gst_bin_remove(pipeline_, h264depay_);
        }

        if (h264dec_) {
          gst_bin_remove(pipeline_, h264dec_);
        }

        if (videosink_) {
          gst_bin_remove(pipeline_, videosink_);
        }


      }
    }

  private:
    std::string camera_name;
    int camera_index_;
    QComboBox* resolutions_selector;
    std::list<std::string> resolutions_;
    QWidget* video_widget;
    GstBin* pipeline_ = nullptr;

    GstElement* video_rtp_udpsrc_ = nullptr;
    GstElement* video_rtcp_udpsrc_ = nullptr;
    GstElement* video_rtcp_udpsink_ = nullptr;
    GstElement* rtpbin_ = nullptr;
    GstElement* h264depay_ = nullptr;
    GstElement* h264dec_ = nullptr;
    GstElement* videosink_ = nullptr;

};


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

  BOOST_LOG_TRIVIAL(info) << "Calling gst_pipeline_new()...";
  GstElement* pipeline = gst_pipeline_new(NULL);

  GstBus* bus = gst_element_get_bus((GstElement*)pipeline);
  g_signal_connect (bus, "message::error", G_CALLBACK (cb_error), pipeline);
  g_signal_connect (bus, "message::warning", G_CALLBACK (cb_warning), pipeline);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (cb_state), pipeline);
  g_signal_connect (bus, "message::eos", G_CALLBACK (cb_eos), NULL);
  gst_bus_add_signal_watch (bus);
  gst_object_unref (bus);

  BOOST_LOG_TRIVIAL(info) << "Calling gst_element_factory_make(\"rtpbin\")...";
  GstElement* rtpbin = gst_element_factory_make("rtpbin", NULL);
  ASSERT_NOT_NULL(rtpbin);
  BOOST_LOG_TRIVIAL(info) << "rtpbin:" << rtpbin;

  g_object_set (rtpbin, "latency", 200, "do-retransmission", TRUE,
      "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);


  GMainLoop* loop = g_main_loop_new(NULL, FALSE);
  ASSERT_NOT_NULL(loop);

  auto loop_runner_func = [&] {
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() starting.";
      g_main_loop_run (loop);
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() finished ok.";
  };

  auto loop_runner_thread_future = std::async(std::launch::async, loop_runner_func);


  //BOOST_LOG_TRIVIAL(info) << "Calling gst_element_factory_make(\"videotestsrc\")...";
  //GstElement* src = gst_element_factory_make("videotestsrc", NULL);

  //BOOST_LOG_TRIVIAL(info) << "Calling gst_element_factory_make(\"d3dvideosink\")...";
  //GstElement* sink = gst_element_factory_make("d3dvideosink", NULL);

  BOOST_LOG_TRIVIAL(info) << "Calling gst_bin_add_many()...";
  gst_bin_add_many(GST_BIN_CAST(pipeline),
     rtpbin,
     NULL);

  g_signal_connect(rtpbin, "pad-added", G_CALLBACK(pad_added_handler), nullptr);


  BOOST_LOG_TRIVIAL(info) << "Created gstreamer pipeline.";

  int debug_port_nr;
  std::string server_host;
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("debug-port", boost::program_options::value<int>(&debug_port_nr)->default_value(12346), "debug port")
    ("server-host", boost::program_options::value<std::string>(&server_host)->default_value("localhost"), "server-host")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);  

  QApplication app{argc, argv};

  QMainWindow main_window;
  QStatusBar* status_bar = new QStatusBar();
  main_window.setStatusBar(status_bar);

  QWidget* central_widget = new QWidget(&main_window);
  main_window.setCentralWidget(central_widget);
  central_widget->setStyleSheet("background-color: blue;");
  QHBoxLayout* central_layout = new QHBoxLayout(central_widget);

  QHBoxLayout* left_menu_layout = new QHBoxLayout();
  central_layout->addLayout(left_menu_layout);

  QLabel* server_ping_label = new QLabel();
  server_ping_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  server_ping_label->setText("ping: n/a");
  server_ping_label->setStyleSheet("background-color: red; border: none;");
  left_menu_layout->addWidget(server_ping_label);

  QGridLayout* camera_views_layout = new QGridLayout();
  std::mutex camera_views_lock;
  central_layout->addLayout(camera_views_layout);

  std::map<std::string, // camera name
           CameraView*  // camera view
           > camera_views;



  QTcpSocket* server_socket = new QTcpSocket(&main_window);
  QTimer* server_socket_connect_timer = new QTimer(&main_window);  
  QTimer* server_socket_ping_timer = new QTimer(&main_window);  

  auto getConnectionTypeToUse = [&]{ 
      QThread* main_window_thread = main_window.thread();
      QThread* current_thread = QThread::currentThread();
      if (current_thread == main_window_thread) {
        return Qt::DirectConnection;
      } else {
        return Qt::BlockingQueuedConnection;
      }
  };


  auto isConnectedToServer = [&]() {
      bool is_connected_to_server;
      QMetaObject::invokeMethod(&main_window, [&]() {
        if (server_socket->state() == QAbstractSocket::SocketState::ConnectedState) {
          return true;
        } else {
          return false;
        }
      },
       getConnectionTypeToUse(),
       &is_connected_to_server
       );
      return is_connected_to_server;
  };


  auto showStatusBarMessage = [&](const std::string& msg, int timeout=3000) {
      QMetaObject::invokeMethod(&main_window, [=]() {
          status_bar->showMessage(QString::fromStdString(msg), timeout);
        },
        getConnectionTypeToUse()        
        );
  };

  std::function<void(void)> connectToServer;
  connectToServer = [&] {
      std::ostringstream msg;
      msg << "Trying to connect to the server...";
      showStatusBarMessage(msg.str(), 0);
      BOOST_LOG_TRIVIAL(info) << msg.str();
      try {
        server_socket->connectToHost(QString::fromStdString(server_host), 20000);
      }
      catch (std::exception& e) {
        std::ostringstream msg;
        msg << "connectToServer() failed with an exception (I'll retry in few seconds): " << e.what();
        showStatusBarMessage(msg.str());
        BOOST_LOG_TRIVIAL(info) << msg.str();
        server_socket_connect_timer->singleShot(10000, connectToServer);
      }
  };


  auto last_ping_send_time = std::chrono::steady_clock::now();
  auto sendPingMessage = [&] {
    last_ping_send_time = std::chrono::steady_clock::now();
    server_socket->write("ping\n");
  };


  main_window.connect(server_socket, &QTcpSocket::connected, &main_window, [&]() {
      std::ostringstream msg;
      msg << "Connected to the server.";
      showStatusBarMessage(msg.str());
      BOOST_LOG_TRIVIAL(info) << msg.str();
      sendPingMessage();
    });

  main_window.connect(server_socket, &QTcpSocket::disconnected, &main_window, [&]() {
      std::ostringstream msg;
      msg << "Disconnected from the server.";
      showStatusBarMessage(msg.str());
      BOOST_LOG_TRIVIAL(info) << msg.str();
      server_ping_label->setText("ping: n/a");

      gst_element_set_state(pipeline, GST_STATE_NULL);

      for (auto& item : camera_views) {
        CameraView* camera_view = item.second;
        camera_views_layout->removeWidget(camera_view);
        delete camera_view;
      }
      camera_views.clear();

     });

  main_window.connect(server_socket, &QTcpSocket::stateChanged, &main_window, [&](QAbstractSocket::SocketState socketState) {
      BOOST_LOG_TRIVIAL(info) << "server connection stateChanged: " << socketState;
      if (socketState == QAbstractSocket::SocketState::UnconnectedState) {
        server_socket_connect_timer->singleShot(10000, connectToServer);
      }
     });

  main_window.connect(server_socket, &QTcpSocket::readyRead, &main_window, [&]() {
      while (server_socket->canReadLine()) {
        QByteArray response = server_socket->readLine();
        std::string response_as_str(response.data(), response.size());
        while(!response_as_str.empty() && response_as_str[response_as_str.size()-1] == '\n' ) {
          response_as_str.erase(response_as_str.size()-1);
        }
        if (response_as_str == "pong") {
          std::chrono::milliseconds ping_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_ping_send_time);
          std::ostringstream ping_label_text;
          ping_label_text << "ping: " << ping_time_ms;
          server_ping_label->setText(QString::fromStdString(ping_label_text.str()));
          server_socket_connect_timer->singleShot(10000, sendPingMessage);
        } else {
          BOOST_LOG_TRIVIAL(info) << "Got a message from the server: " << response_as_str;
          try {
            boost::json::value response_value = boost::json::parse(response_as_str); 
            const boost::json::object& response_obj = response_value.as_object();
            std::string response_type(response_obj.at("type").as_string());

            if (response_type == "cameras") {
              const boost::json::array& cameras = response_obj.at("cameras").as_array();
              BOOST_LOG_TRIVIAL(info) << "Got " << cameras.size() << " cameras from the server";

              int camera_index = 0;
              boost::json::array camera_responses;
              for (const boost::json::value& item : cameras) {
                const boost::json::object& camera = item.as_object();
                std::string camera_name(camera.at("name").as_string());
                auto find = camera_views.find(camera_name);
                if (find == camera_views.end()) {
                  CameraView* new_camera_view = new CameraView();
                  boost::json::object camera_response_msg = new_camera_view->initialize(
                    server_host,
                    GST_BIN_CAST(pipeline),
                    rtpbin,
                    camera,
                    camera_index);
                  camera_responses.push_back(std::move(camera_response_msg));
                  camera_views_layout->addWidget(new_camera_view);
                  camera_views[camera_name] = new_camera_view;

                }
              }
              if (camera_views.size() > 0) {
                std::ostringstream msg;
                msg << "Got " << camera_views.size() << " cameras from the server.";
                BOOST_LOG_TRIVIAL(info) << msg.str();
                showStatusBarMessage(msg.str());
              }

              BOOST_LOG_TRIVIAL(info) << "Calling gst_debug_bin_to_dot_file()";
              GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)pipeline, GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE, "client.dot");
              BOOST_LOG_TRIVIAL(info) << "gst_debug_bin_to_dot_file() finished ok";

              GstStateChangeReturn result = gst_element_set_state(pipeline, GST_STATE_PLAYING);
              BOOST_LOG_TRIVIAL(info) << "Started the gstreamer pipeline. result:" << result;
              if (result == GST_STATE_CHANGE_FAILURE) {
                throw std::runtime_error("Failed to start the gstreamer pipeline!");
              }

              boost::json::object response;
              response["type"] = "welcome-response";
              response["cameras"] = std::move(camera_responses);
              std::string response_str = boost::json::serialize(response) + "\n";
              auto bytes_written = server_socket->write(response_str.data(), response_str.size());
              if (bytes_written < 0) {
                BOOST_LOG_TRIVIAL(info) << "Failed to reply to the server's welcome message.";
                server_socket->close();
              }

            } else {
              BOOST_LOG_TRIVIAL(error) << "Got an unknown response-type '" << response_type << "' from the server! The raw response string was: '" << response_as_str << "'";
              std::ostringstream msg;
              msg << "Got an unknown response-type '" << response_type << "' from the server! The raw response string was: '" << response_as_str << "'";
              throw std::runtime_error(msg.str());
            }

          } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Got this exception when trying to parse the response from the server: " << e.what() << std::endl
                                    << "The raw response string was: '" << response_as_str << "'";
            server_socket->close();
          }

        }

      }
     });

  server_socket_connect_timer->singleShot(100, connectToServer);



  boost::asio::io_context ctx;

  std::shared_ptr<LineBasedServer> debug_port;
  if (debug_port_nr > 0) {
    BOOST_LOG_TRIVIAL(info) << "client starting debug listen port at " << debug_port_nr;
    debug_port = std::make_unique<LineBasedServer>(
      ctx,
      debug_port_nr,

     [&](boost::asio::ip::tcp::socket& sock) {
      std::ostringstream msg;
      msg << "Got a new debug-port connection from '" << sock.remote_endpoint() << "'";
      BOOST_LOG_TRIVIAL(info) << msg.str();
      showStatusBarMessage(msg.str());
      return "";
     },

     [&](boost::asio::ip::tcp::socket& sock) {
      std::ostringstream msg;
      msg << "Lost the debug-port connection from '" << sock.remote_endpoint() << "'";
      BOOST_LOG_TRIVIAL(info) << msg.str();
      showStatusBarMessage(msg.str());
     },
 
     [&](boost::asio::ip::tcp::socket& sock, const std::string& request) {
      BOOST_LOG_TRIVIAL(info) << "Got a debug port message from '" << sock.remote_endpoint() << "': " << request;
      std::string response;
      if (request == "ping") {
        response = "pong";

      } else if (request == "is connected to server") {
        if (isConnectedToServer()) {
          response = "true";
        } else {
          response = "false";
        }

      } else if (request == "get cameras") {
        boost::json::array camera_list;
        std::lock_guard guard(camera_views_lock);
        for(const auto& item : camera_views) {
          boost::json::object camera;
          camera["name"] = item.first;
          CameraView* camera_view = item.second; 
          boost::json::array resolutions;
          for(const std::string& resolution : camera_view->getResolutions()) {
            resolutions.emplace_back(resolution);
          }
          camera["resolutions"] = std::move(resolutions);
          camera_list.push_back(std::move(camera));
        }
        response = boost::json::serialize(camera_list);

      } else {
        try {
          boost::json::object request_obj = boost::json::parse(request).as_object();
          std::string request_type(request_obj.at("type").as_string());
          if (request_type == "select_camera") {
            std::string camera_name(request_obj.at("camera").as_string());
            std::string resolution(request_obj.at("resolution").as_string());
            response = "ok";
          } else {
            std::ostringstream msg;
            msg << "Unknown request type: '" << request_type << "'";
            throw std::runtime_error(msg.str());
          }

        } catch(const std::exception& e) {
          std::ostringstream msg;
          msg << "ERROR: Got this error: '" << e.what() << "' while trying to handle the request '" << request << "'";
          response = msg.str();
        }
      }
      return response;
    }
    
    );
  
  }
  BOOST_LOG_TRIVIAL(info) << "client starting up.";
  auto asio_main_future = std::async(std::launch::async, [&ctx]{ctx.run();});

  main_window.show();

  int result = app.exec();
  ctx.stop();

  BOOST_LOG_TRIVIAL(info) << "client shutting down. Calling asio_main_future.get();";
  asio_main_future.get();
  
  BOOST_LOG_TRIVIAL(info) << "client shutting down. Calling loop_runner_thread_future.get();";
  g_main_loop_quit(loop);

  loop_runner_thread_future.get();
  

  return result;
}


}

int main(int argc, char** argv) {
  int exitcode = snowrobot::main(argc, argv);
  BOOST_LOG_TRIVIAL(info) << "client exiting with exitcode " << exitcode;
}
#endif
