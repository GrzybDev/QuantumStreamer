#pragma once
#include "HttpClient.hpp"

class StreamingServerSocket : boost::asio::noncopyable
{
public:
	StreamingServerSocket(boost::asio::io_service& ioService, USHORT port,
	                      const std::shared_ptr<HttpClient>& httpClient);

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;
	std::shared_ptr<HttpClient> httpClient_;

	void WaitForConnection();
};
