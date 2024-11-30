#pragma once

class HttpClient : public std::enable_shared_from_this<HttpClient>
{
public:
	HttpClient(boost::asio::io_context& ioContext);

	boost::beast::http::response<boost::beast::http::dynamic_body> Get(std::string urlString);
private:
	boost::asio::ip::tcp::resolver resolver_;
	boost::beast::tcp_stream client_;
};
