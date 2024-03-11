#include <iostream>
#include "linebasedserver.h"

namespace snowrobot {


LineBasedServer::LineBasedServer(boost::asio::io_context& ctx, boost::asio::ip::port_type admin_port_nr,
                ConnectionMadeFunc connection_made_func,
                ConnectionLostFunc connection_lost_func,
                RequestReceivedFunc response_func
) : connection_made_func_(connection_made_func),
    connection_lost_func_(connection_lost_func),
    response_func_(response_func)
{
  boost::asio::co_spawn(ctx, this->listen(ctx, admin_port_nr), [](std::exception_ptr eptr)
  {
    try
    {
      if (eptr) {
        std::rethrow_exception(eptr);
      }
    }
    catch(const std::exception& e)
    {
      std::cout << "LineBasedServer::LineBasedServer() this->listen() failed " << e.what() << std::endl;
    }
  });
}


boost::asio::awaitable<void> LineBasedServer::listen(boost::asio::io_context& ctx, boost::asio::ip::port_type admin_port_nr)
{
  
  boost::asio::ip::tcp::acceptor acceptor(ctx, {boost::asio::ip::tcp::v4(), admin_port_nr}, false);
#ifdef _WIN32
  typedef boost::asio::detail::socket_option::boolean<BOOST_ASIO_OS_DEF(SOL_SOCKET), SO_EXCLUSIVEADDRUSE> excluse_address;
  acceptor.set_option(excluse_address(true));
#else
  acceptor.set_option(boost::asio::socket_base::reuse_address(false));
#endif

  try {
    for (;;)
    {
      boost::asio::co_spawn(
        acceptor.get_executor(),
        this->handle_connection(co_await acceptor.async_accept(boost::asio::use_awaitable)),
        boost::asio::detached);
    }
  }
  catch(const boost::system::system_error& e) {
    std::string info = boost::diagnostic_information(e, true);
    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::listen() got a boost::system::system_error exception: " << info << ". code:" << e.code();
    throw;
  }
  catch(const std::exception& e) {
    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::listen() got an exception: " << e.what();
    throw;
  }
}

boost::asio::awaitable<void> LineBasedServer::watchdog(std::chrono::steady_clock::time_point& deadline)
{
  boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
  auto now = std::chrono::steady_clock::now();
  while (deadline > now)
  {
    timer.expires_at(deadline);
    co_await timer.async_wait(boost::asio::use_awaitable);
    now = std::chrono::steady_clock::now();
  }

  BOOST_LOG_TRIVIAL(info) << "LineBasedServer::watchdog() exiting, since we timed out";
  // When this function exists, the echo() function will also exit, since all io-operations will be aborted. See
  // the note about the "||" operator in the handle_connection() function.
}



class ConnectionLostRAII {
  public:
    ConnectionLostRAII(ConnectionLostFunc connection_lost_func, boost::asio::ip::tcp::socket& sock):
      connection_lost_func_(connection_lost_func), socket_(sock) {
    }
    ~ConnectionLostRAII() {
      try {
        this->connection_lost_func_(this->socket_);
      }
      catch(const std::exception& e) {
        std::cerr << "~ConnectionLostRAII(): got an exception: " << e.what() << std::endl;
      }
    }
  private:
    ConnectionLostFunc connection_lost_func_;
    boost::asio::ip::tcp::socket& socket_;
};


boost::asio::awaitable<void> LineBasedServer::handle_connection(boost::asio::ip::tcp::socket sock)
{
  BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() got a new connection from " << sock.remote_endpoint();
  ConnectionLostRAII connection_lost_raii(this->connection_lost_func_, sock);


  std::chrono::steady_clock::time_point deadline{};
  try {
    std::string welcome_message = this->connection_made_func_(sock);
    if (!welcome_message.empty()) {
      welcome_message += "\n";
      // Write the response back to the socket
      co_await boost::asio::async_write(sock, boost::asio::buffer(welcome_message), boost::asio::use_awaitable);
    }


    co_await (
      this->handle_requests(sock, deadline)
      
      // The "||" operator is overloaded by boost::asio::experimental::awaitable_operators and works like this:
      // When either of the handle_requests() or watchdog() coroutines exit, the io-operations in the other function will fail with
      // an boost::asio::error::operation_aborted error. We use this to implement a timeout on the connection: if no message
      // has been received within a certain time, the watchdog() coroutine will exit. The watchdog deadline time is
      // extended by handle_requests() each time it receives a message.
      ||  

      this->watchdog(deadline)
      );

    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() co_await() returned with no errors.";

  }
  catch(const boost::asio::multiple_exceptions& e) {
    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() got an boost::asio::multiple_exceptions: " << e.what();
    if (e.first_exception()) {
      try {
        std::rethrow_exception(e.first_exception());
      }
      catch(const std::exception& fe) {
        BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() e.first_exception(): " << fe.what();
      }
    }
  }
  catch(const boost::system::system_error& e) {
    std::string info = boost::diagnostic_information(e, true);
    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() got boost::system::system_error exception: " << info << ". code:" << e.code();
  }
  catch(const std::exception& e) {
    BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_connection() got an exception: " << e.what();
  }
}

boost::asio::awaitable<void> LineBasedServer::handle_requests(boost::asio::ip::tcp::socket& sock,
                                                              std::chrono::steady_clock::time_point& deadline)
{
  boost::asio::streambuf streambuf;

  for (;;)
  {
    try {
      // Extend the watchdog deadline with a few more seconds. 
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);

      auto n = co_await boost::asio::async_read_until(sock, streambuf, "\n", boost::asio::use_awaitable);
      if (n > 0) {
        std::string request;
        std::istream is(&streambuf);
        std::getline(is, request, '\n');

        // we got a request, so call our callback function to get the response.
        if (request.size() > 0) {
          // remove windows carriage return
          if (request[request.size()-1] == '\r') {
            request.erase(request.size()-1);
          }
        }
        std::string response = this->response_func_(sock, request);
        request.clear();
        if (!response.empty()) {
          if (response[response.size()-1] != '\n') {
            response += "\n";
          }
          // Write the response back to the socket
          co_await boost::asio::async_write(sock, boost::asio::buffer(response), boost::asio::use_awaitable);
        }
      }
    }
    catch(const boost::system::system_error& e) {
      if (e.code().value() == boost::asio::error::operation_aborted) {
      BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_requests() got an operation_aborted exception, which means that the connection timed out.";
      } else {
      std::string info = boost::diagnostic_information(e, true);
      BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_requests() got a boost::system::system_error exception: " << info << ". code:" << e.code();
      }
      break;
    }
    catch(const std::exception& e) {
      BOOST_LOG_TRIVIAL(info) << "LineBasedServer::handle_requests() got an '" << typeid(e).name() << "' exception (I'll rethrow it): " << e.what();
      throw;
    }
  }
}

}
