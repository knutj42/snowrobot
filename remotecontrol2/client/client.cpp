#include <iostream>
#include <gst/gst.h>

int
main (int argc, char *argv[])
{
   g_print("Hello World!");
  GstElement *pipeline;
  GstBus *bus;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Build the pipeline */
  pipeline =
      gst_parse_launch
      ("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  
  bool isfinished = false;
  while (!isfinished) {
    GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);

    if (msg == nullptr) {
      std::cerr << "msg == nullptr!" << std::endl;
      //isfinished = true;
    } else {
      std::cerr << "Got a msg! msg->type:" << msg->type  << std::endl;
      /* See next tutorial for proper error message handling/parsing */
      if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        std::cout << "An error occurred" << std::endl;
        g_error ("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
            "variable set for more details.");
        isfinished = true;
      }
      gst_message_unref (msg);
    }
  }
  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  std::cout << "Finished ok" << std::endl;
  return 0;
}
