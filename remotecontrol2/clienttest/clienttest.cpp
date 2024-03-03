#include <cstdio>
#include <future>

#include <winsock2.h>

#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QTNetwork/QTcpSocket>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <boost/log/trivial.hpp>

#include "../common/gst_wrappers.h"


namespace snowrobot {

int main(int   argc,
      char *argv[]) {


  const gchar *nano_str;
  guint major, minor, micro, nano;

  gst_init (&argc, &argv);

  gst_version (&major, &minor, &micro, &nano);

  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";

  printf ("This program is linked against GStreamer %d.%d.%d %s\n",
          major, minor, micro, nano_str);

  QApplication app{argc, argv};

  QMainWindow main_window;
  QStatusBar* status_bar = new QStatusBar();
  main_window.setStatusBar(status_bar);

  QWidget* central_widget = new QWidget(&main_window);
  main_window.setCentralWidget(central_widget);
  central_widget->setStyleSheet("background-color: blue;");

  main_window.show();

  GMainLoop* loop = g_main_loop_new(NULL, FALSE);
  ASSERT_NOT_NULL(loop);

  GstElement* pipeline = gst_pipeline_new(NULL);
  ASSERT_NOT_NULL(pipeline);





  //GstElement* video_source = gst_element_factory_make("videotestsrc", NULL);
  GstElement* video_source = gst_element_factory_make(
#ifdef __linux__ 
        "v4l2src",
#else
        "ksvideosrc",
        //"videotestsrc"
#endif
        NULL);


  ASSERT_NOT_NULL(video_source);
  ASSERT_TRUE(gst_bin_add((GstBin*)pipeline, video_source));

  GstElement* queue = gst_element_factory_make("queue", NULL);
  ASSERT_NOT_NULL(queue);
  ASSERT_TRUE(gst_bin_add((GstBin*)pipeline, queue));

  GstElement* videosink = gst_element_factory_make("d3dvideosink", NULL);
  ASSERT_NOT_NULL(videosink);
  ASSERT_TRUE(gst_bin_add((GstBin*)pipeline, videosink));

  BOOST_LOG_TRIVIAL(info) << "Calling gst_caps_new_simple()";
  /*GstCaps* video_caps = gst_caps_from_string("video/x-raw",
  "width", G_TYPE_INT, 640,
  "height", G_TYPE_INT, 360,
  "framerate", GST_TYPE_FRACTION, 15, 1,
   NULL);
*/

  GstCaps* video_caps = gst_caps_from_string("video/x-raw, format=YUY2, width=640, height=360, framerate=30/1, pixel-aspect-ratio=1/1");

  BOOST_LOG_TRIVIAL(info) << "gst_caps_from_string() returned " << video_caps;


  ASSERT_TRUE(gst_element_link_filtered(video_source, queue, video_caps));
  //ASSERT_TRUE(gst_element_link(video_source, queue));
  ASSERT_TRUE(gst_element_link(queue, videosink));

  //ASSERT_TRUE(gst_element_link(video_source, videosink));

  auto winId = central_widget->winId();
  BOOST_LOG_TRIVIAL(info) << "central_widget->winId() " << winId;
  gst_video_overlay_set_window_handle((GstVideoOverlay*)videosink, winId);


  auto loop_runner_func = [&] {
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() starting.";
      g_main_loop_run (loop);
      BOOST_LOG_TRIVIAL(info) << "loop_runner_func() finished ok.";
  };

  auto loop_runner_thread_future = std::async(std::launch::async, loop_runner_func);

  GstStateChangeReturn result = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  BOOST_LOG_TRIVIAL(info) << "Started the gstreamer pipeline. result:" << result;
  if (result == GST_STATE_CHANGE_FAILURE) {
    throw std::runtime_error("Failed to start the gstreamer pipeline!");
  }

  int qt_app_result = app.exec();

  BOOST_LOG_TRIVIAL(info) << "client shutting down. Calling loop_runner_thread_future.get();";
  g_main_loop_quit(loop);
  loop_runner_thread_future.get();


  return 0;

}


}


int main(int   argc,
      char *argv[]) {
  return snowrobot::main(argc, argv);
}