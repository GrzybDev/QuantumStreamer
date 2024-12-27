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
		std::string episode = target.substr(1, target.find_first_of("/", 1) - 1);
		boost::replace_all(episode, "%20", " ");
		const std::string action = target.substr(target.find_first_of("/", 1) + 1);

		const std::string manifestUrl = videoList.GetEpisodeURL(episode);

		if (manifestUrl.empty())
			response_.result(http::status::not_found);
		else
		{
			if (action == "manifest")
				SendManifest(episode, manifestUrl);
			else
				SendFragment(episode, action, manifestUrl);
		}
	}
	else
		response_.result(http::status::bad_request);

	WriteResponse();
}

void StreamingServerConnection::SendManifest(std::string episodeId, std::string manifestUrl)
{
	SmoothStreaming smoothStreaming = SmoothStreaming::GetInstance();
	std::string clientManifest = smoothStreaming.GetClientManifest(episodeId);

	if (clientManifest.empty())
	{
		BOOST_LOG_TRIVIAL(debug) << "Client manifest not found locally, fetching from server...";

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
		ostream(response_.body()) << clientManifest;
}

void StreamingServerConnection::SendFragment(std::string episode, std::string action, std::string manifestUrl)
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

		int captions_res = type.find("_captions");
		bool isCaptions = captions_res != std::string::npos;

		auto smoothStreaming = SmoothStreaming::GetInstance();
		auto responseBytes = smoothStreaming.GetFragment(episode, type, std::stoll(bitrate), std::stoll(starttime));

		if (responseBytes.empty())
		{
			BOOST_LOG_TRIVIAL(debug) << "Fragment " << starttime << " from track " << type << " for episode " << episode
 << " not found locally, fetching from server...";

			boost::system::result<boost::url_view> urlView(manifestUrl);
			std::string fragmentUrl = urlView.value().scheme();
			fragmentUrl += "://" + urlView.value().host();

			std::string newPath = urlView.value().path();
			boost::replace_all(newPath, "manifest", action);

			fragmentUrl += "/" + newPath;

			try
			{
				auto response = httpClient_->Get(fragmentUrl);
				responseBytes = buffers_to_string(response.body().data());
			}
			catch (...)
			{
				response_.result(http::status::internal_server_error);
			}

			if (isCaptions)
			{
				auto responseBytesData = new char[responseBytes.size()];
				memcpy(responseBytesData, responseBytes.data(), responseBytes.size());

				unsigned int moofSize;
				memcpy(&moofSize, responseBytesData, 4);
				moofSize = _byteswap_ulong(moofSize);

				auto moof = new char[moofSize];
				memcpy(moof, responseBytesData, moofSize);

				unsigned int mdatSize;
				memcpy(&mdatSize, responseBytesData + moofSize, 4);
				mdatSize = _byteswap_ulong(mdatSize);

				auto mdat = new char[mdatSize];
				memcpy(mdat, responseBytesData + moofSize, mdatSize);

				delete[] responseBytesData;

				// Skip 8 bytes and read it as a string
				std::string mdatString(mdat + 8, mdatSize - 8);

				auto subtitleOverride = SubtitleOverride::GetInstance();
				auto newString = subtitleOverride.GetSubtitleOverride(episode, type, mdatString);

				// Update the mdat size
				mdatSize = newString.size() + 8;

				// Update the mdat
				delete[] mdat;
				mdat = new char[mdatSize];

				// Write mdatSize in big-endian
				unsigned int mdatSizeBE = _byteswap_ulong(mdatSize);
				memcpy(mdat, &mdatSizeBE, 4);
				memcpy(mdat + 4, "mdat", 4);
				memcpy(mdat + 8, newString.c_str(), newString.size());

				responseBytes = std::string(moof, moofSize) + std::string(mdat, mdatSize);

				delete[] moof;
				delete[] mdat;
			}
		}

		ostream(response_.body()) << responseBytes;
	}
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
