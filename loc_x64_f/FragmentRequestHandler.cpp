#include "pch.hpp"
#include "FragmentRequestHandler.hpp"

#include "OfflineStreaming.hpp"
#include "VideoList.hpp"

using Poco::Logger;
using Poco::StreamCopier;
using Poco::Timespan;
using Poco::URI;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Util::Application;

FragmentRequestHandler::FragmentRequestHandler(std::string episodeId, std::string bitrate, std::string type,
                                               std::string startTime) : _episodeId(std::move(episodeId)),
                                                                        _bitrate(std::move(bitrate)),
                                                                        _type(std::move(type)),
                                                                        _startTime(std::move(startTime))
{
}

void FragmentRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
{
	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string manifestUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _type, _startTime);

	if (manifestUrl.empty())
	{
		response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();
	std::string localFragment = offlineStreaming.GetLocalFragment(_episodeId, _type, _bitrate, _startTime);

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
		HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setTimeout(Timespan(10, 0)); // Set a timeout for the request

		// Send the request and get the response
		session.sendRequest(manifestRequest);

		HTTPResponse manifestResponse;
		std::istream& manifestResponseStream = session.receiveResponse(manifestResponse);

		std::ostringstream buffer;
		StreamCopier::copyStream(manifestResponseStream, buffer);
		std::string bodyStr = buffer.str();

		response.setStatus(manifestResponse.getStatus());

		for (const auto& [key, value] : manifestResponse)
			response.set(key, value);

		std::ostream& responseBody = response.send();
		responseBody.write(bodyStr.data(), static_cast<LONGLONG>(bodyStr.size()));
	}
	else
	{
		response.setContentLength(static_cast<LONGLONG>(localFragment.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(localFragment.data(), static_cast<LONGLONG>(localFragment.size()));
	}
}
