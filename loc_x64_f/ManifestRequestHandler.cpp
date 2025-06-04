#include "pch.hpp"
#include "ManifestRequestHandler.hpp"

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

ManifestRequestHandler::ManifestRequestHandler(std::string episodeId) : _episodeId(std::move(episodeId))
{
}

void ManifestRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
{
	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string manifestUrl = videoList.getManifestUrl(_episodeId);

	if (manifestUrl.empty())
	{
		response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();
	std::string localManifest = offlineStreaming.GetLocalClientManifest(_episodeId);

	if (localManifest.empty())
	{
		Logger& logger = Logger::get("Server");

		if (app.config().getBool("Server.OfflineMode", false))
		{
			logger.warning("Offline mode is enabled, but the requested manifest is not available locally: %s",
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

		try
		{
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

			// Set a timeout for the request (Game seems to use 20 seconds till it tries to retry)
			session.setTimeout(Timespan(20, 0));

			// Send the request and get the response
			session.sendRequest(manifestRequest);

			HTTPResponse manifestResponse;
			std::istream& manifestResponseStream = session.receiveResponse(manifestResponse);

			std::ostringstream buffer;
			StreamCopier::copyStream(manifestResponseStream, buffer);
			std::string bodyStr = buffer.str();

			auto responseStatus = manifestResponse.getStatus();

			if (responseStatus != HTTPResponse::HTTP_OK)
			{
				logger.error("Failed to fetch client manifest! Remote server returned %s status code.",
				             std::to_string(responseStatus));
				logger.trace(bodyStr);
			}

			response.setStatus(responseStatus);

			for (const auto& header : manifestResponse)
				response.set(header.first, header.second);

			std::ostream& responseBody = response.send();
			responseBody.write(bodyStr.data(), static_cast<LONGLONG>(bodyStr.size()));
		}
		catch (Poco::Exception& ex)
		{
			logger.error("Exception happened when handling client manifest request! (%s)", ex.displayText());
			response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
			response.send();
		}
	}
	else
	{
		response.setContentLength(static_cast<LONGLONG>(localManifest.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(localManifest.data(), static_cast<LONGLONG>(localManifest.size()));
	}
}
