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


#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <boost/program_options.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/parse.hpp>


namespace snowrobot {



class CameraView : public QFrame {
  public:
    CameraView(const std::string& camera_name,
               const std::list<std::string>& resolutions) : QFrame(), resolutions_(resolutions) {
      this->camera_name = camera_name;
      QVBoxLayout* layout = new QVBoxLayout();
      this->setLayout(layout);
      resolutions_selector = new QComboBox();
      for(const std::string& resolution: resolutions) {
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

    } 

    const std::list<std::string>& getResolutions() const {return resolutions_;}
    auto getVideoWidgetWinId() {
      return this->video_widget->winId();
    }

  private:
    std::string camera_name;
    QComboBox* resolutions_selector;
    std::list<std::string> resolutions_;
    QWidget* video_widget;
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
  GstElement_ptr pipeline(gst_pipeline_new("xvoverlay"));

  BOOST_LOG_TRIVIAL(info) << "Calling gst_element_factory_make()...";
  GstElement_ptr src(gst_element_factory_make("videotestsrc", NULL));

  BOOST_LOG_TRIVIAL(info) << "Calling gst_element_factory_make()...";
  GstElement_ptr sink(gst_element_factory_make("d3dvideosink", NULL));

  BOOST_LOG_TRIVIAL(info) << "Calling gst_bin_add_many()..." << pipeline.get() << "  " << src.get() << "  " << sink.get();
  gst_bin_add_many(GST_BIN_CAST(pipeline.get()), src.get(), sink.get(), NULL);


  BOOST_LOG_TRIVIAL(info) << "Calling gst_element_link()...";
  gst_element_link(src.get(), sink.get());

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
      BOOST_LOG_TRIVIAL(info) << "showStatusBarMessage() running. msg: '" << msg << "'";

      QMetaObject::invokeMethod(&main_window, [=]() {
          BOOST_LOG_TRIVIAL(info) << "showStatusBarMessage() lambda running";
          //BOOST_LOG_TRIVIAL(info) << "showStatusBarMessage() lambda running. msg: '" << msg << "'";
          status_bar->showMessage(QString::fromStdString(msg), timeout);
        },
        getConnectionTypeToUse()        
        );
      BOOST_LOG_TRIVIAL(info) << "showStatusBarMessage() finished. msg: '" << msg << "'";
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

  main_window.connect(server_socket, &QTcpSocket::connected, &main_window, [&]() {
      std::ostringstream msg;
      msg << "Connected to the server.";
      showStatusBarMessage(msg.str());
      BOOST_LOG_TRIVIAL(info) << msg.str();

      std::string get_cameras_msg = "get cameras\n";
      auto written_bytes = server_socket->write(get_cameras_msg.data(), get_cameras_msg.size());
      if (written_bytes != get_cameras_msg.size()) {
        std::ostringstream error_msg;
        error_msg << "Failed to send all the bytes! written_bytes=" << written_bytes << "  get_cameras_msg.size():" << get_cameras_msg.size();
        BOOST_LOG_TRIVIAL(error) << error_msg.str();
        server_socket->close();
      }
      // Note that the server communication is async, so the answer will be handled in the QTcpSocket::readyRead read handler. 
    });

  main_window.connect(server_socket, &QTcpSocket::disconnected, &main_window, [&]() {
      std::ostringstream msg;
      msg << "Disconnected from the server.";
      showStatusBarMessage(msg.str());
      BOOST_LOG_TRIVIAL(info) << msg.str();
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
        //BOOST_LOG_TRIVIAL(info) << "Got a response from the server: " << response_as_str;
        try {
          boost::json::value response_value = boost::json::parse(response_as_str); 
          const boost::json::object& response_obj = response_value.as_object();
          std::string response_type(response_obj.at("type").as_string());

          if (response_type == "cameras") {
            const boost::json::array& cameras = response_obj.at("cameras").as_array();
            BOOST_LOG_TRIVIAL(info) << "Got " << cameras.size() << " cameras from the server";

            for (const boost::json::value& item : cameras) {
              const boost::json::object& camera = item.as_object();
              std::string camera_name(camera.at("name").as_string());
              auto find = camera_views.find(camera_name);
              if (find == camera_views.end()) {
                std::list<std::string> resolutions;
                for (const boost::json::value& resolution: camera.at("resolutions").as_array()) {
                  std::string resolution_str(resolution.as_string());
                  if (resolution_str.find("video") != 0) {
                    // this isn't a video-resolution, so skip it. We aren't interested in still-frame resolutions.
                    continue;
                  }
                  resolutions.push_back(resolution_str);
                }
                CameraView* new_camera_view = new CameraView(camera_name, resolutions);
                camera_views[camera_name] = new_camera_view;
                camera_views_layout->addWidget(new_camera_view);
                auto winId = new_camera_view->getVideoWidgetWinId();
                BOOST_LOG_TRIVIAL(info) << "new_camera_view->getVideoWidgetWinId(): " << winId;

                gst_video_overlay_set_window_handle((GstVideoOverlay*)sink.get(), winId);

                // run the pipeline
                BOOST_LOG_TRIVIAL(info) << "starting the gstreamer pipeline";

                GstStateChangeReturn sret = gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);
                if (sret == GST_STATE_CHANGE_FAILURE) {
                  BOOST_LOG_TRIVIAL(error) << "failed to start the gstreamer pipeline" << sret;
                  gst_element_set_state(pipeline.get(), GST_STATE_NULL);
                  // Exit application
                  //QTimer::singleShot(0, QApplication::activeWindow(), SLOT(quit()));
                }
              }
            }
            if (camera_views.size() > 0) {
              std::ostringstream msg;
              msg << "Got " << camera_views.size() << " cameras from the server.";
              BOOST_LOG_TRIVIAL(info) << msg.str();
              showStatusBarMessage(msg.str());
            }


          } else {
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
     });

  server_socket_connect_timer->singleShot(100, connectToServer);



  boost::asio::io_context ctx;

  std::shared_ptr<LineBasedServer> debug_port;
  if (debug_port_nr > 0) {
    BOOST_LOG_TRIVIAL(info) << "client starting debug listen port at " << debug_port_nr;
    debug_port = std::make_unique<LineBasedServer>(
      ctx,
      debug_port_nr,

     [&](const std::string& connection_token) {
      std::ostringstream msg;
      msg << "Got a new debug-port connection from '" << connection_token << "'";
      BOOST_LOG_TRIVIAL(info) << msg.str();
      showStatusBarMessage(msg.str());
     },

     [&](const std::string& connection_token) {
      std::ostringstream msg;
      msg << "Lost the debug-port connection from '" << connection_token << "'";
      BOOST_LOG_TRIVIAL(info) << msg.str();
      showStatusBarMessage(msg.str());
     },
 
     [&](const std::string& connection_token, const std::string& request) {
      BOOST_LOG_TRIVIAL(info) << "Got a debug port message from '" << connection_token << "': " << request;
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
            std::string camera_name(request_obj.at("camare").as_string());
            std::string resolution(request_obj.at("resolution").as_string());

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
  std::thread asio_main_thread([&ctx]{ctx.run();});

  main_window.show();

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
  int exitcode = snowrobot::main(argc, argv);
  BOOST_LOG_TRIVIAL(info) << "client exiting with exitcode " << exitcode;
}
