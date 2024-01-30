#include <winsock2.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <boost/program_options.hpp>
#include <gst/gst.h>

#include "../common/linebasedserver.h"
#include <thread>

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
    debug_port = std::make_unique<LineBasedServer>(ctx, debug_port_nr, [](const std::string request) {
      std::string response;
      if (request == "ping") {
        response = "pong";
      } else {
        response = "ERROR: unknown request '" + request + "'";
      }
      return response;
    });
  }

  LineBasedServer command_port(ctx, command_port_nr,
    [&](const std::string request) {
      std::string response;
      if (request == "ping") {
        response = "pong";
      } else if (request == "start camera search") {
        response = "started a camera device search";
      } else if (request == "get cameras") {
        response = "camera device search in progress...";
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

  //ctx.stop();

  BOOST_LOG_TRIVIAL(info) << "Calling asio_main_thread.join();";
  asio_main_thread.join();

  BOOST_LOG_TRIVIAL(info) << "server shutting down.";
  return 0;
}


}



int main(int argc, char** argv) {
    return snowrobot::main(argc, argv);
}
