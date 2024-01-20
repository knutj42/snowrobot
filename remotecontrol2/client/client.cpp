
// https://github.com/KubaO/stackoverflown/tree/master/questions/dynamic-widget-10790454
#include <cmath>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <array>
#include <boost/asio/asio.hpp>
// Interface


class ClockView : public QLabel
{
public:
    explicit ClockView(QWidget* parent = nullptr) : QLabel(parent)
    {
        static int ctr = 0;
        setText(QString::number(ctr++));
    }
};

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void populateViewGrid();

private:
    static constexpr int N = 10;

    QWidget central{this};
    QGridLayout centralLayout{&central};
    std::array<QFrame, N> frames;

    std::array<ClockView, N> clockViews;
    std::array<QVBoxLayout, N> layouts;
};

// Implementation

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setCentralWidget(&central);

    const int n = ceil(sqrt(N));
    for (int i = 0; i < N; ++ i) {
        frames[i].setFrameShape(QFrame::StyledPanel);
        centralLayout.addWidget(&frames[i], i/n, i%n, 1, 1);
    }

    populateViewGrid();
}

void MainWindow::populateViewGrid()
{
    for (int i = 0; i < N; ++ i) {
        layouts[i].addWidget(&clockViews[i]);
        frames[i].setLayout(&layouts[i]);
    }
}

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    MainWindow w;
    w.show();
    return app.exec();
}


/*
#include <iostream>
#include <gst/gst.h>

int
main (int argc, char *argv[])
{
   g_print("Hello World!");
  GstElement *pipeline;
  GstBus *bus;

  // Initialize GStreamer
  gst_init (&argc, &argv);

  // Build the pipeline
  pipeline =
      gst_parse_launch
      ("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
      NULL);

  // Start playing
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  // Wait until error or EOS
  bus = gst_element_get_bus (pipeline);
  
  bool isfinished = false;
  while (!isfinished) {
    GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);

    if (msg == nullptr) {
      std::cerr << "msg == nullptr!" << std::endl;
      //isfinished = true;
    } else {
      std::cerr << "Got a msg! msg->type:" << msg->type  << std::endl;
      if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        std::cout << "An error occurred" << std::endl;
        g_error ("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
            "variable set for more details.");
        isfinished = true;
      }
      gst_message_unref (msg);
    }
  }
  // Free resources 
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  std::cout << "Finished ok" << std::endl;
  return 0;
}
*/