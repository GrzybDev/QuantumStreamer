#include "pch.hpp"
#include "SubtitleFragmentRequestHandler.hpp"

#include "OfflineStreaming.hpp"
#include "SubtitleOverride.hpp"
#include "VideoList.hpp"

using Poco::Logger;
using Poco::StreamCopier;
using Poco::Timespan;
using Poco::URI;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Util::Application;

SubtitleFragmentRequestHandler::SubtitleFragmentRequestHandler(std::string episodeId, std::string bitrate,
                                                               std::string langCode,
                                                               std::string startTime) :
	_episodeId(std::move(episodeId)),
	_bitrate(std::move(bitrate)),
	_langCode(std::move(langCode)),
	_startTime(std::move(startTime))
{
}

void SubtitleFragmentRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
{
	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string fragmentUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _langCode + "_captions", _startTime);

	if (fragmentUrl.empty())
	{
		response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();
	std::string localFragment = offlineStreaming.GetLocalFragment(_episodeId, _langCode + "_captions", _bitrate,
	                                                              _startTime);

	if (localFragment.empty())
	{
		Logger& logger = Logger::get("Server");

		if (app.config().getBool("Server.OfflineMode", false))
		{
			logger.warning("Offline mode is enabled, but the requested subtitle fragment is not available locally: %s",
			               _episodeId);

			response.setStatus(HTTPResponse::HTTP_NOT_ACCEPTABLE);
			response.send();
			return;
		}

		// Call the fragment URL keeping all headers, query parameters, and body intact
		// The only thing we need to change is the Host header to the fragment URL's host

		// Parse fragment URL to extract the host
		URI uri(fragmentUrl);
		const std::string& fragmentHost = uri.getHost();

		try
		{
			HTTPRequest fragmentRequest(HTTPRequest::HTTP_GET, fragmentUrl, HTTPMessage::HTTP_1_1);

			// Copy headers from the original request to the fragment request
			for (const auto& [key, value] : request)
			{
				if (key != "Host")
				{
					// Skip Host header, we'll set it later
					fragmentRequest.set(key, value);
				}
			}

			// Set the Host header to the fragment URL's host
			fragmentRequest.set("Host", fragmentHost);

			// Send the request to the fragment URL
			HTTPClientSession session(uri.getHost(), uri.getPort());

			// Set a timeout for the request (Game seems to use 20 seconds till it tries to retry)
			session.setTimeout(Timespan(20, 0));

			// Send the request and get the response
			session.sendRequest(fragmentRequest);

			HTTPResponse fragmentResponse;
			std::istream& fragmentResponseStream = session.receiveResponse(fragmentResponse);

			std::ostringstream buffer;
			StreamCopier::copyStream(fragmentResponseStream, buffer);
			std::string bodyStr = buffer.str();
			std::string bodyStrNew = processSubtitleData(bodyStr);

			auto responseStatus = fragmentResponse.getStatus();

			if (responseStatus != HTTPResponse::HTTP_OK)
			{
				logger.error("Failed to fetch subtitle fragment! Remote server returned %s status code.",
				             std::to_string(responseStatus));
				logger.trace(bodyStr);
			}

			response.setStatus(responseStatus);

			for (const auto& [key, value] : fragmentResponse)
				response.set(key, value);

			// Calculate Content-Length
			response.setContentLength(static_cast<long long>(bodyStrNew.size()));

			std::ostream& responseBody = response.send();
			responseBody.write(bodyStrNew.data(), static_cast<long long>(bodyStrNew.size()));
		}
		catch (Poco::Exception& ex)
		{
			logger.error("Exception happened when handling client fragment request! (%s)", ex.displayText());
			response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
			response.send();
		}
	}
	else
	{
		std::string newLocalFragment = processSubtitleData(localFragment);
		response.setContentLength(static_cast<long long>(newLocalFragment.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(newLocalFragment.data(), static_cast<long long>(newLocalFragment.size()));
	}
}

std::string SubtitleFragmentRequestHandler::processSubtitleData(const std::string& data) const
{
	const auto bytesData = new char[data.size()];
	memcpy(bytesData, data.data(), data.size()); // NOLINT(bugprone-not-null-terminated-result)

	unsigned int moofSize;
	memcpy(&moofSize, bytesData, 4);
	moofSize = _byteswap_ulong(moofSize);

	const auto moofBlock = new char[moofSize];
	memcpy(moofBlock, bytesData, moofSize);

	unsigned int mdatSize;
	memcpy(&mdatSize, bytesData + moofSize, 4);
	mdatSize = _byteswap_ulong(mdatSize);

	auto mdatBlock = new char[mdatSize];
	memcpy(mdatBlock, bytesData + moofSize, mdatSize);

	delete[] bytesData;

	// Skip 8 bytes and read it as string
	std::string subtitleData(mdatBlock + 8, mdatSize - 8);

	const Application& app = Application::instance();
	SubtitleOverride& subtitleOverride = app.getSubsystem<SubtitleOverride>();

	const std::string newSubtitleData = subtitleOverride.OverrideSubtitles(
		_episodeId, _langCode + "_captions", subtitleData,
		_startTime == "80080000");

	mdatSize = static_cast<unsigned int>(newSubtitleData.size() + 8);

	// delete original mdat
	delete[] mdatBlock;
	mdatBlock = new char[mdatSize];

	// Write mdatSize in big-endian
	const unsigned int mdatSizeBe = _byteswap_ulong(mdatSize);
	memcpy(mdatBlock, &mdatSizeBe, 4);
	memcpy(mdatBlock + 4, "mdat", 4); // NOLINT(bugprone-not-null-terminated-result)
	memcpy(mdatBlock + 8, newSubtitleData.c_str(), newSubtitleData.size());

	std::string newData = std::string(moofBlock, moofSize) + std::string(mdatBlock, mdatSize);

	delete[] moofBlock;
	delete[] mdatBlock;

	return newData;
}
