#ifndef SNOWROBOT_REMOTECONTROL_COMMON_LINEBASEDSERVER_h
#define SNOWROBOT_REMOTECONTROL_COMMON_LINEBASEDSERVER_h

#include <boost/asio/buffer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/log/trivial.hpp>

using namespace boost::asio::experimental::awaitable_operators;


namespace snowrobot {


// This class implements a simple line-based server. It opens a tcp/ip listen socket on the specified portnumber and start
// accepting connections. Once a string of bytes ending with '\n' is received the callback function that was specified in
// the constructor is called with the received string (minus the \n character and any trailing '\r' character).
// This is intended to be used to low-volume command-streams where we want to trace latency for readability and ease of
// debugging. It is nice to be able to use telnet to manually send messages to such servers.
class LineBasedServer {
  public:
    LineBasedServer(boost::asio::io_context& ctx, boost::asio::ip::port_type admin_port_nr,
                   std::function<std::string(std::string)> response_func);
  private:
    boost::asio::awaitable<void> listen(boost::asio::io_context& ctx, boost::asio::ip::port_type admin_port_nr);

    boost::asio::awaitable<void> watchdog(std::chrono::steady_clock::time_point& deadline);

    boost::asio::awaitable<void> handle_connection(boost::asio::ip::tcp::socket sock);
    boost::asio::awaitable<void> handle_requests(boost::asio::ip::tcp::socket& sock, std::chrono::steady_clock::time_point& deadline);
    std::function<std::string(std::string)> response_func;
};

}


#endif
