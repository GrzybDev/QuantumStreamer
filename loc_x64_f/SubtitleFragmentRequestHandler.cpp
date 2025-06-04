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

	std::string manifestUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _langCode + "_captions", _startTime);

	if (manifestUrl.empty())
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
		if (app.config().getBool("Server.OfflineMode", false))
		{
			Logger& logger = Logger::get("Server");
			logger.warning("Offline mode is enabled, but the requested fragment is not available locally: %s",
			               _episodeId);

			response.setStatus(HTTPResponse::HTTP_NOT_ACCEPTABLE);
			response.send();
			return;
		}

		// Call the manifest URL keeping all headers, query parameters, and body intact
		// The only thing we need to change is the Host header to the manifest URL's host

		// Parse manifest URL to extract the host
		URI uri(manifestUrl);
		const std::string& manifestHost = uri.getHost();

		HTTPRequest manifestRequest(HTTPRequest::HTTP_GET, manifestUrl, HTTPMessage::HTTP_1_1);

		// Copy headers from the original request to the manifest request
		for (const auto& [key, value] : request)
		{
			if (key != "Host")
			{
				// Skip Host header, we'll set it later
				manifestRequest.set(key, value);
			}
		}
		// Set the Host header to the manifest URL's host
		manifestRequest.set("Host", manifestHost);

		// Send the request to the manifest URL
		HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setTimeout(Timespan(10, 0)); // Set a timeout for the request

		// Send the request and get the response
		session.sendRequest(manifestRequest);

		HTTPResponse manifestResponse;
		std::istream& manifestResponseStream = session.receiveResponse(manifestResponse);

		std::ostringstream buffer;
		StreamCopier::copyStream(manifestResponseStream, buffer);
		std::string bodyStr = buffer.str();
		std::string bodyStrNew = processSubtitleData(bodyStr);

		response.setStatus(manifestResponse.getStatus());

		for (const auto& header : manifestResponse)
			response.set(header.first, header.second);

		// Calculate Content-Length
		response.setContentLength(static_cast<LONGLONG>(bodyStrNew.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(bodyStrNew.data(), static_cast<LONGLONG>(bodyStrNew.size()));
	}
	else
	{
		std::string newLocalFragment = processSubtitleData(localFragment);
		response.setContentLength(static_cast<LONGLONG>(newLocalFragment.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(newLocalFragment.data(), static_cast<LONGLONG>(newLocalFragment.size()));
	}
}

std::string SubtitleFragmentRequestHandler::processSubtitleData(const std::string& data) const
{
	const auto bytesData = new char[data.size()];
	memcpy(bytesData, data.data(), data.size()); // NOLINT(bugprone-not-null-terminated-result)

	UINT moofSize;
	memcpy(&moofSize, bytesData, 4);
	moofSize = _byteswap_ulong(moofSize);

	const auto moofBlock = new char[moofSize];
	memcpy(moofBlock, bytesData, moofSize);

	UINT mdatSize;
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

	mdatSize = static_cast<UINT>(newSubtitleData.size() + 8);

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
