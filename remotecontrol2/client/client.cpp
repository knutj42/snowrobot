#include <winsock2.h>

#include <cmath>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QTcpSocket>
#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <typeinfo>


#include <gst/gst.h>
#include <boost/program_options.hpp>

#include "../common/linebasedserver.h"

namespace snowrobot {




class LeftMenuBar : public QFrame {
  public:
    LeftMenuBar(QWidget *parent) : QFrame(parent) {
      setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

      setFrameShape(QFrame::StyledPanel);
      setStyleSheet("background-color: blue;");

      server_ping_label_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
      server_ping_label_.setText("ping: n/a");
      server_ping_label_.setStyleSheet("background-color: red; border: none;");
      layout_.addWidget(&server_ping_label_);

      server_ping2_label_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
      server_ping2_label_.setText("ping2: n/a");
      server_ping2_label_.setStyleSheet("background-color: brown; border: none;");
      layout_.addWidget(&server_ping2_label_);

      layout_.addWidget(&spacer_label_);

    }
  private:
    QVBoxLayout layout_{this};
    QLabel server_ping_label_;
    QLabel server_ping2_label_;
    QLabel spacer_label_;
};


class CameraViews : public QFrame {
  public:
    CameraViews(QWidget *parent) : QFrame(parent) {
      setFrameShape(QFrame::StyledPanel);
      setStyleSheet("background-color: pink;");
    }
  private:
    QVBoxLayout layout_{this};
};


class MainWindow : public QMainWindow
{
  public:
    explicit MainWindow(const QString& server_host);

  private:
    QWidget central_{this};
    QGridLayout central_layout_{&central_};

    LeftMenuBar left_menu_bar_{&central_};
    CameraViews camera_views_{&central_};

    void ConnectToServer() {

    }


    QTcpSocket server_socket_{this};
    QString server_host_;

};



MainWindow::MainWindow(const QString& server_host) :
  QMainWindow(nullptr), server_host_(server_host)
{
  setCentralWidget(&central_);
  setStyleSheet("background-color: green;");

  central_layout_.addWidget(&left_menu_bar_, 0, 0);
  central_layout_.addWidget(&camera_views_, 0, 1);

  connect(&server_socket_, &QTcpSocket::connected, this, [=]() {
      BOOST_LOG_TRIVIAL(info) << "server connected.";
     });
  connect(&server_socket_, &QTcpSocket::disconnected, this, [=]() {
      BOOST_LOG_TRIVIAL(info) << "server disconnected.";
     });
  connect(&server_socket_, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState socketState) {
      BOOST_LOG_TRIVIAL(info) << "server connection stateChanged: " << socketState;
     });


  server_socket_.connectToHost(server_host_, 20000);
}




int main(int argc, char** argv)
{
  int debug_port_nr;
  std::string server_host;
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("debug-port", boost::program_options::value<int>(&debug_port_nr)->default_value(0), "debug port")
    ("server-host", boost::program_options::value<std::string>(&server_host)->default_value("localhost"), "server-host")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);  

  QApplication app{argc, argv};
  MainWindow w(QString::fromStdString(server_host));

  boost::asio::io_context ctx;

  std::unique_ptr<LineBasedServer> debug_port;
  if (debug_port_nr > 0) {
    BOOST_LOG_TRIVIAL(info) << "client starting debug listen port at " << debug_port_nr;
    debug_port = std::make_unique<LineBasedServer>(ctx, debug_port_nr, [](const std::string request) {
    std::string response;
    if (request == "ping") {
      response = "pong";
    } else if (request == "is connected to server") {
      response = "false";
    } else {
      response = "ERROR: unknown request '" + request + "'";
    }
    return response;
    });
  
  }
  BOOST_LOG_TRIVIAL(info) << "client starting up.";
  std::thread asio_main_thread([&ctx]{ctx.run();});

  w.show();
  int result = app.exec();
  ctx.stop();

  BOOST_LOG_TRIVIAL(info) << "client shutting down. Calling asio_main_thread.join();";
  asio_main_thread.join();
  return result;

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

}

int main(int argc, char** argv) {
  return snowrobot::main(argc, argv);
}
