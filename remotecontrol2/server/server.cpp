#include "../common/linebasedserver.h"
#include "../common/gst_wrappers.h"

#include <thread>

#include <chrono>
#include <iostream>
#include <typeinfo>

#include <winsock2.h>
#include <boost/program_options.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>



namespace snowrobot {


  


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

  // At least on Windows11, the gst_device_monitor_get_devices() function sometimes doesn't find the itegrated camera on my laptop.
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

  std::unique_ptr<LineBasedServer> debug_port;
  if (debug_port_nr > 0) {
    debug_port = std::make_unique<LineBasedServer>(
      ctx,
      debug_port_nr,
    
     [](const std::string& connection_token) {
      BOOST_LOG_TRIVIAL(info) << "Got a new debug-port connection from '" << connection_token << "'";
     },

     [](const std::string& connection_token) {
      BOOST_LOG_TRIVIAL(info) << "Lost the debug-port connection from '" << connection_token << "'";
     },

     [](const std::string& connection_token, const std::string& request) {
      BOOST_LOG_TRIVIAL(info) << "Got a debug-port message from '" << connection_token << "': " << request;
      std::string response;
      if (request == "ping") {
        response = "pong";
      } else {
        response = "ERROR: unknown request '" + request + "'";
      }
      return response;
    });
  }

  LineBasedServer command_port(
     ctx,
     command_port_nr,
     
     [](const std::string& connection_token) {
      BOOST_LOG_TRIVIAL(info) << "Got a new connection from '" << connection_token << "'";
     },

     [](const std::string& connection_token) {
      BOOST_LOG_TRIVIAL(info) << "Lost the connection from '" << connection_token << "'";
     },

     [&](const std::string& connection_token, const std::string& request) {
      BOOST_LOG_TRIVIAL(info) << "Got a message from '" << connection_token << "': " << request;
      std::string response;
      if (request == "ping") {
        response = "pong";

      } else if (request == "get cameras") {
        boost::json::array cameras;
        for (GList* devIter = g_list_first(devices.get()); devIter != nullptr; devIter=g_list_next(devIter)) {
          GstDevice * device = (GstDevice*) devIter->data;
          if (device == nullptr) {
            continue;
          }

          std::string device_class = string_from_gchar(gst_device_get_device_class(device));
          if (device_class != "Video/Source") {
            continue;
          }
          boost::json::object camera;

          std::string display_name = string_from_gchar(gst_device_get_display_name(device));
          camera["name"] = display_name;

          auto device_gst_caps = make_GstCaps_ptr(gst_device_get_caps(device));
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
          gst_caps_foreach (device_gst_caps.get(),
                        for_each_caps2,
                        &resolutions);
          camera["resolutions"] = std::move(resolutions);
          cameras.push_back(std::move(camera));
        }

        boost::json::object response_msg;
        response_msg["type"] = "cameras";
        response_msg["cameras"] = std::move(cameras);
        response = boost::json::serialize(response_msg);
        BOOST_LOG_TRIVIAL(info) << "Responding with this camera list to '" << connection_token << "': " << response;


      } else if (request == "stop") {
        ctx.stop();
        response = "stopping server";

      } else {
        response = "ERROR: unknown request '" + request + "'";
      }
      return response;
    }
  );

  BOOST_LOG_TRIVIAL(info) << "server starting up.";
  std::thread asio_main_thread([&ctx]{ctx.run();});

  BOOST_LOG_TRIVIAL(info) << "Calling asio_main_thread.join();";
  asio_main_thread.join();

  BOOST_LOG_TRIVIAL(info) << "server shutting down.";
  return 0;
}


}



int main(int argc, char** argv) {
    return snowrobot::main(argc, argv);
}
