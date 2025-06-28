#include "pch.hpp"
#include "manifest.hpp"

#include "../subsystems/offline_streaming.hpp"
#include "../subsystems/video_list.hpp"

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

ManifestRequestHandler::ManifestRequestHandler(std::string episode_id) : episode_id_(std::move(episode_id))
{
}

void ManifestRequestHandler::handleWithLogging(HTTPServerRequest& request, HTTPServerResponse& response)
{
	Logger& logger = Logger::get("Network");

	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string manifestUrl = videoList.getManifestUrl(episode_id_);

	if (manifestUrl.empty())
	{
		response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();

	if (std::string localManifest = offlineStreaming.getLocalClientManifest(episode_id_); localManifest.empty())
	{
		if (app.config().getBool("Server.OfflineMode", false))
		{
			logger.warning("Offline mode is enabled, but the requested client manifest is not available locally: %s",
			               episode_id_);

			response.setStatusAndReason(HTTPResponse::HTTP_NOT_ACCEPTABLE);
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
			logger.trace("Fetching client manifest from remote server (%s)...", manifestUrl);
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

			// Set a timeout for the request
			session.setTimeout(Timespan(REMOTE_TIMEOUT, 0));

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
				logger.trace("Remote server response: %s", bodyStr);
			}

			response.setStatusAndReason(responseStatus);

			for (const auto& [key, value] : manifestResponse)
				response.set(key, value);

			std::ostream& responseBody = response.send();
			responseBody.write(bodyStr.data(), static_cast<long long>(bodyStr.size()));
		}
		catch (Poco::Exception& ex)
		{
			logger.error(
				"An exception occurred while fetching client manifest from the remote server! [episode_id: %s] (%s)",
				episode_id_, ex.displayText());
			response.setStatusAndReason(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
			response.send();
		}
	}
	else
	{
		logger.trace("Serving local client manifest for episode %s...", episode_id_);
		response.setContentLength(static_cast<long long>(localManifest.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(localManifest.data(), static_cast<long long>(localManifest.size()));
	}
}
