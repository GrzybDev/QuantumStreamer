#include "pch.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

StreamingServerConnection::StreamingServerConnection(tcp::socket socket, const std::shared_ptr<HttpClient>& httpClient) : socket_(std::move(socket))
{
	httpClient_ = httpClient;
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

	// Expected URL format: /<episode>/manifest,/<episode>/<bitrate>/<fragment>

	if (request_.method() == http::verb::get)
	{
		const std::string target = request_.target();
		const std::string episode = target.substr(1, target.find_first_of("/", 1) - 1);
		const std::string action = target.substr(target.find_first_of("/", 1) + 1);

		const std::string manifestUrl = videoList.GetEpisodeURL(episode);
		if (manifestUrl.empty())
			response_.result(http::status::not_found);
		else
		{
			if (action == "manifest")
			{
				auto response = httpClient_->Get(manifestUrl);
				response_.body() = response.body();
			}
			else
			{
				boost::system::result<boost::url_view> urlView(manifestUrl);
				std::string fragmentUrl = urlView.value().scheme();
				fragmentUrl += "://" + urlView.value().host();

				std::string newPath = urlView.value().path();
				boost::replace_all(newPath, "manifest", action);

				fragmentUrl += "/" + newPath;

				auto response = httpClient_->Get(fragmentUrl);
				response_.body() = response.body();
			}
		}
	}
	else
		response_.result(http::status::bad_request);

	WriteResponse();
}


void StreamingServerConnection::WriteResponse()
{
	auto self = shared_from_this();

	BOOST_LOG_TRIVIAL(debug) << boost::format("%s %s [%s %s]") % request_.method_string() % request_.target() % response_.result_int() % response_.result();
	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
		});
}
