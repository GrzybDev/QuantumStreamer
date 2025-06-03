#include "pch.hpp"
#include "SubtitleFragmentRequestHandler.hpp"

#include "OfflineStreaming.hpp"
#include "SubtitleOverride.hpp"
#include "VideoList.hpp"

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

void SubtitleFragmentRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                                   Poco::Net::HTTPServerResponse& response)
{
	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string manifestUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _langCode + "_captions", _startTime);

	if (manifestUrl.empty())
	{
		response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();
	std::string localFragment = offlineStreaming.GetLocalFragment(_episodeId, _langCode + "_captions", _bitrate, _startTime);

	if (localFragment.empty())
	{
		// Call the manifest URL keeping all headers, query parameters, and body intact
		// The only thing we need to change is the Host header to the manifest URL's host

		// Parse manifest URL to extract the host
		Poco::URI uri(manifestUrl);
		std::string manifestHost = uri.getHost();

		Poco::Net::HTTPRequest manifestRequest(Poco::Net::HTTPRequest::HTTP_GET, manifestUrl,
			Poco::Net::HTTPMessage::HTTP_1_1);
		// Copy headers from the original request to the manifest request
		for (const auto& header : request)
		{
			if (header.first != "Host")
			{
				// Skip Host header, we'll set it later
				manifestRequest.set(header.first, header.second);
			}
		}
		// Set the Host header to the manifest URL's host
		manifestRequest.set("Host", manifestHost);

		// Send the request to the manifest URL
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setTimeout(Poco::Timespan(10, 0)); // Set a timeout for the request

		// Send the request and get the response
		std::ostream& manifestRequestStream = session.sendRequest(manifestRequest);

		Poco::Net::HTTPResponse manifestResponse;
		std::istream& manifestResponseStream = session.receiveResponse(manifestResponse);

		std::ostringstream buffer;
		Poco::StreamCopier::copyStream(manifestResponseStream, buffer);
		std::string bodyStr = buffer.str();
		std::string bodyStrNew = processSubtitleData(bodyStr);

		response.setStatus(manifestResponse.getStatus());

		for (const auto& header : manifestResponse)
			response.set(header.first, header.second);

		// Calculate Content-Length
		response.setContentLength(bodyStrNew.size());

		std::ostream& responseBody = response.send();
		responseBody.write(bodyStrNew.data(), bodyStrNew.size());
	}
	else
	{
		std::string newLocalFragment = processSubtitleData(localFragment);
		response.setContentLength(newLocalFragment.size());

		std::ostream& responseBody = response.send();
		responseBody.write(newLocalFragment.data(), newLocalFragment.size());
	}
}

std::string SubtitleFragmentRequestHandler::processSubtitleData(std::string& data)
{
	auto bytesData = new char[data.size()];
	memcpy(bytesData, data.data(), data.size());

	UINT moofSize;
	memcpy(&moofSize, bytesData, 4);
	moofSize = _byteswap_ulong(moofSize);

	auto moofBlock = new char[moofSize];
	memcpy(moofBlock, bytesData, moofSize);

	UINT mdatSize;
	memcpy(&mdatSize, bytesData + moofSize, 4);
	mdatSize = _byteswap_ulong(mdatSize);

	auto mdatBlock = new char[mdatSize];
	memcpy(mdatBlock, bytesData + moofSize, mdatSize);

	delete[] bytesData;

	// Skip 8 bytes and read it as string
	std::string subtitleData(mdatBlock + 8, mdatSize - 8);

	Application& app = Application::instance();
	SubtitleOverride& subtitleOverride = app.getSubsystem<SubtitleOverride>();

	std::string newSubtitleData = subtitleOverride.OverrideSubtitles(_episodeId, _langCode + "_captions", subtitleData,
	                                                                 _startTime == "80080000");

	mdatSize = newSubtitleData.size() + 8;

	// delete original mdat
	delete[] mdatBlock;
	mdatBlock = new char[mdatSize];

	// Write mdatSize in big-endian
	unsigned int mdatSizeBE = _byteswap_ulong(mdatSize);
	memcpy(mdatBlock, &mdatSizeBE, 4);
	memcpy(mdatBlock + 4, "mdat", 4);
	memcpy(mdatBlock + 8, newSubtitleData.c_str(), newSubtitleData.size());

	std::string newData = std::string(moofBlock, moofSize) + std::string(mdatBlock, mdatSize);

	delete[] moofBlock;
	delete[] mdatBlock;

	return newData;
}
