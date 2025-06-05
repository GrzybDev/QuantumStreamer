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

	std::string fragmentUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _type, _startTime);

	if (fragmentUrl.empty())
	{
		response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

	OfflineStreaming& offlineStreaming = app.getSubsystem<OfflineStreaming>();
	std::string localFragment = offlineStreaming.GetLocalFragment(_episodeId, _type, _bitrate, _startTime);

	if (localFragment.empty())
	{
		Logger& logger = Logger::get("Server");

		if (app.config().getBool("Server.OfflineMode", false))
		{
			logger.warning("Offline mode is enabled, but the requested fragment is not available locally: %s",
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

			auto responseStatus = fragmentResponse.getStatus();

			if (responseStatus != HTTPResponse::HTTP_OK)
			{
				logger.error("Failed to fetch fragment! Remote server returned %s status code.",
				             std::to_string(responseStatus));
				logger.trace(bodyStr);
			}

			response.setStatus(responseStatus);

			for (const auto& [key, value] : fragmentResponse)
				response.set(key, value);

			std::ostream& responseBody = response.send();
			responseBody.write(bodyStr.data(), static_cast<long long>(bodyStr.size()));
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
		response.setContentLength(static_cast<long long>(localFragment.size()));

		std::ostream& responseBody = response.send();
		responseBody.write(localFragment.data(), static_cast<long long>(localFragment.size()));
	}
}
