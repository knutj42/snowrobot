#include <chrono>
#include <iostream>
#include <memory>
#include <typeinfo>

#include <boost/asio/buffer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/log/trivial.hpp>

using namespace boost::asio::experimental::awaitable_operators;


boost::asio::awaitable<void> echo(boost::asio::ip::tcp::socket& sock, std::chrono::steady_clock::time_point& deadline)
{
  char data[5];
  for (;;)
  {
    try {
      deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
      auto n = co_await sock.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
      BOOST_LOG_TRIVIAL(info) << "echo() read " << n << " bytes";
      co_await boost::asio::async_write(sock, boost::asio::buffer(data, n), boost::asio::use_awaitable);
      BOOST_LOG_TRIVIAL(info) << "echo() sent " << n << " bytes";
    }
    catch(const boost::system::system_error& e) {
      if (e.code().value() == boost::asio::error::operation_aborted) {
        BOOST_LOG_TRIVIAL(info) << "echo() got an operation_aborted exception, which means that the connection timed out.";
      } else {
        std::string info = boost::diagnostic_information(e, true);
        BOOST_LOG_TRIVIAL(info) << "echo() got boost::system::system_error exception: " << info << ". code:" << e.code();
      }
      break;
    }
    catch(const std::exception& e) {
      BOOST_LOG_TRIVIAL(info) << "echo() got an '" << typeid(e).name() << "' exception (I'll rethrow it): " << e.what();
      throw;
    }

  }
}

boost::asio::awaitable<void> watchdog(std::chrono::steady_clock::time_point& deadline)
{
  boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
  auto now = std::chrono::steady_clock::now();
  while (deadline > now)
  {
    timer.expires_at(deadline);
    co_await timer.async_wait(boost::asio::use_awaitable);
    now = std::chrono::steady_clock::now();
  }
  //throw boost::system::system_error(std::make_error_code(std::errc::timed_out));
  BOOST_LOG_TRIVIAL(info) << "watchdog() exiting, since we timed out";
}


boost::asio::awaitable<void> handle_connection(boost::asio::ip::tcp::socket sock)
{
  BOOST_LOG_TRIVIAL(info) << "handle_connection() got a new connection from " << sock.remote_endpoint();
  std::chrono::steady_clock::time_point deadline{};
  try {
    co_await (
      echo(sock, deadline)
       ||
      watchdog(deadline)
      );

    BOOST_LOG_TRIVIAL(info) << "handle_connection() co_await() returned with no errors.";

  }
  catch(const boost::asio::multiple_exceptions& e) {
    BOOST_LOG_TRIVIAL(info) << "handle_connection() got an boost::asio::multiple_exceptions: " << e.what();
    if (e.first_exception()) {
      try {
        std::rethrow_exception(e.first_exception());
      }
      catch(const std::exception& fe) {
        BOOST_LOG_TRIVIAL(info) << "handle_connection() e.first_exception(): " << fe.what();
      }
    }
  }
  catch(const boost::system::system_error& e) {
    std::string info = boost::diagnostic_information(e, true);
    BOOST_LOG_TRIVIAL(info) << "handle_connection() got boost::system::system_error exception: " << info << ". code:" << e.code();
  }
  catch(const std::exception& e) {
    BOOST_LOG_TRIVIAL(info) << "handle_connection() got an exception: " << e.what();
  }
}

boost::asio::awaitable<void> listen(boost::asio::ip::tcp::acceptor& acceptor)
{
  try {

    for (;;)
    {
      co_spawn(
          acceptor.get_executor(),
          handle_connection(co_await acceptor.async_accept(boost::asio::use_awaitable)),
          boost::asio::detached);
    }
  }
  catch(const std::exception& e) {
    BOOST_LOG_TRIVIAL(info) << "listen() got an exception: " << e.what();
  }

}

int main()
{
  BOOST_LOG_TRIVIAL(info) << "server starting up.";
  boost::asio::io_context ctx;
  boost::asio::ip::tcp::acceptor acceptor(ctx, {boost::asio::ip::tcp::v4(), 12345});
  BOOST_LOG_TRIVIAL(info) << "server created acceptor.";
  co_spawn(ctx, listen(acceptor), boost::asio::detached);
  BOOST_LOG_TRIVIAL(info) << "server calling ctx.run().";
  ctx.run();
  BOOST_LOG_TRIVIAL(info) << "server finished ok.";
}
