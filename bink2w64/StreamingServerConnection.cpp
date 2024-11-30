#include "pch.hpp"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

StreamingServerConnection::StreamingServerConnection(tcp::socket socket,
                                                     const std::shared_ptr<HttpClient>& httpClient) : socket_(
	std::move(socket))
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
			std::string baseLocalPath = "videos\\episodes\\" + episode + "\\";

			if (action == "manifest")
			{
				std::string manifestPath = baseLocalPath + "manifest.ismc";

				if (!boost::filesystem::exists(manifestPath))
				{
					try
					{
						auto response = httpClient_->Get(manifestUrl);
						response_.body() = response.body();
					}
					catch (...)
					{
						response_.result(http::status::internal_server_error);
					}
				}
				else
				{
					std::ifstream fileStream(manifestPath, std::ios::binary);
					std::vector<char> fileBytes((std::istreambuf_iterator<char>(fileStream)),
					                            std::istreambuf_iterator<char>());

					ostream(response_.body()) << std::string(fileBytes.begin(), fileBytes.end());
				}
			}
			else
			{
				boost::regex pattern("QualityLevels\\((.*)\\)/Fragments\\((.*)=(.*)\\)");
				boost::smatch match;

				if (!regex_search(action, match, pattern))
					response_.result(http::status::bad_request);
				else
				{
					std::string bitrate = match.str(1);
					std::string type = match.str(2);
					std::string starttime = match.str(3);

					std::string typechar;
					int captions_res = type.find("_captions");

					if (type == "video")
						typechar = "v";
					else if (captions_res != std::string::npos)
						typechar = "t";
					else
						typechar = "a";

					std::string chunkPath = baseLocalPath + type + "\\" + starttime + ".ism" + typechar;

					if (!boost::filesystem::exists(chunkPath))
					{
						boost::system::result<boost::url_view> urlView(manifestUrl);
						std::string fragmentUrl = urlView.value().scheme();
						fragmentUrl += "://" + urlView.value().host();

						std::string newPath = urlView.value().path();
						boost::replace_all(newPath, "manifest", action);

						fragmentUrl += "/" + newPath;

						try
						{
							auto response = httpClient_->Get(fragmentUrl);
							response_.body() = response.body();
						}
						catch (...)
						{
							response_.result(http::status::internal_server_error);
						}
					}
					else
					{
						std::ifstream fileStream(chunkPath, std::ios::binary);
						std::vector<char> fileBytes((std::istreambuf_iterator<char>(fileStream)),
						                            std::istreambuf_iterator<char>());
						ostream(response_.body()) << std::string(fileBytes.begin(), fileBytes.end());
					}
				}
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
