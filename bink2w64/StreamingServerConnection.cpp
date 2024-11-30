#include "pch.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

StreamingServerConnection::StreamingServerConnection(tcp::socket socket) : socket_(std::move(socket))
{
	BOOST_LOG_FUNCTION()
}


void StreamingServerConnection::Start()
{
	ReadRequest();
}

void StreamingServerConnection::ReadRequest()
{
	auto self = shared_from_this();

	async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code error,
		       std::size_t bytesTransferred)
		{
			boost::ignore_unused(bytesTransferred);
			if (!error)
				self->ProcessRequest();
		});
}

void StreamingServerConnection::ProcessRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<h1>It works!</h1>\n";
		break;

	default:
		// We return responses indicating an error if
		// we do not recognize the request method.
		response_.result(http::status::bad_request);
		break;
	}

	WriteResponse();
}


void StreamingServerConnection::WriteResponse()
{
	auto self = shared_from_this();

	BOOST_LOG_TRIVIAL(debug) << boost::format("%s %s [%s %s]") % request_.method_string() % request_.target() %
 response_.result_int() % response_.result();
	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
		});
}
