#include "pch.hpp"

namespace ip = boost::asio::ip; // from <boost/asio.hpp>
using tcp = ip::tcp; // from <boost/asio.hpp>

StreamingServerSocket::StreamingServerSocket(boost::asio::io_service& ioService, const USHORT port):
	acceptor_(ioService, tcp::endpoint(tcp::v4(), port)), socket_(ioService)
{
	WaitForConnection();
	BOOST_LOG_TRIVIAL(info) << "Created HTTP Server (listening on port " << port << ")...";
}

void StreamingServerSocket::WaitForConnection()
{
	acceptor_.async_accept(socket_,
	                       [&](boost::beast::error_code error)
	                       {
		                       if (!error)
			                       std::make_shared<StreamingServerConnection>(std::move(socket_))->Start();

		                       WaitForConnection();
	                       });
}
