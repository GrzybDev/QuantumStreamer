#include "pch.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

HttpClient::HttpClient(boost::asio::io_context& ioContext) : resolver_(ioContext), client_(ioContext)
{
}

http::response<http::dynamic_body> HttpClient::Get(std::string urlString)
{
	// Parse the URL
	boost::system::result<boost::url_view> urlView(urlString);
	std::string host = urlView.value().host();
	std::string port = urlView.value().port();

	if (port.empty())
		port = urlView.value().scheme() == "http" ? "80" : "443";

	// Look up the domain name
	const auto results = resolver_.resolve(host, port);

	// This buffer is used for reading and must be persisted
	beast::flat_buffer buffer;

	// Declare a container to hold the response
	http::response<http::dynamic_body> res;

	beast::error_code ec;

	// Make the connection on the IP address we get from a lookup
	client_.connect(results);

	// Restore the target URL
	std::string target = urlView.value().path();

	if (!urlView.value().query().empty())
		target += "?" + urlView.value().query();

	if (!urlView.value().fragment().empty())
		target += "#" + urlView.value().fragment();

	// Send the HTTP request to the remote host
	http::request<http::string_body> request{ http::verb::get, target, 11 };
	request.set(http::field::host, host);
	request.set(http::field::user_agent, "QuantumStreamer/1.0");
	http::write(client_, request);

	// Receive the HTTP response
	http::read(client_, buffer, res);

	client_.socket().shutdown(tcp::socket::shutdown_both, ec);

	// not_connected happens sometimes
	// so don't bother reporting it.
	//
	if (ec && ec != beast::errc::not_connected)
		throw beast::system_error{ ec };

	return res;
}
