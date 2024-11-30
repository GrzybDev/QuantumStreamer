#pragma once

class StreamingServerSocket : boost::asio::noncopyable
{
public:
	StreamingServerSocket(boost::asio::io_service& ioService, USHORT port);

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;

	void WaitForConnection();
};
