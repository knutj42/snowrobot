#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>



boost::asio::awaitable::awaitable<void> echo(tcp::socket& sock, time_point& deadline)
{
  char data[4196];
  for (;;)
  {
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    auto n = co_await sock.async_read_some(buffer(data), use_awaitable);
    co_await async_write(sock, buffer(data, n), use_awaitable);
  }
}

boost::asio::awaitable<void> watchdog(time_point& deadline)
{
  steady_timer timer(co_await this_coro::executor);
  auto now = std::chrono::steady_clock::now();
  while (deadline > now)
  {
    timer.expires_at(deadline);
    co_await timer.async_wait(use_awaitable);
    now = std::chrono::steady_clock::now();
  }
  throw boost::system::system_error(std::make_error_code(std::errc::timed_out));
}

boost::asio::awaitable<void> handle_connection(tcp::socket sock)
{
  time_point deadline{};
  co_await (echo(sock, deadline) && watchdog(deadline));
}

boost::asio::awaitable<void> listen(tcp::acceptor& acceptor)
{
  for (;;)
  {
    co_spawn(
        acceptor.get_executor(),
        handle_connection(co_await acceptor.async_accept(boost::asio::use_awaitable)),
        detached);
  }
}

int main()
{
  io_context ctx;
  tcp::acceptor acceptor(ctx, {tcp::v4(), 54321});
  co_spawn(ctx, listen(acceptor), boost::asio::detached);
  ctx.run();
}
