#include <winsock2.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <typeinfo>

#include "../common/linebasedserver.h"

namespace snowrobot {

int main(int argc, char** argv)
{
  boost::asio::io_context ctx;

  int admin_port_nr = 12345;
  LineBasedServer admin_port(ctx, admin_port_nr, [](const std::string request) {
    std::string response;
    if (request == "ping") {
      response = "pong";
    } else {
      response = "ERROR: unknown request '" + request + "'";
    }
    return response;
  });

  BOOST_LOG_TRIVIAL(info) << "server starting up.";
  ctx.run();

  return 0;
}


}



int main(int argc, char** argv) {
    return snowrobot::main(argc, argv);
}
