#include "pch.hpp"
#include "fragment.hpp"

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

FragmentRequestHandler::FragmentRequestHandler(std::string episode_id, std::string bitrate, std::string type,
                                               std::string start_time):
	episode_id_(std::move(episode_id)),
	bitrate_(std::move(bitrate)),
	type_(std::move(type)),
	start_time_(std::move(start_time))
{
}

void FragmentRequestHandler::handleWithLogging(HTTPServerRequest& request, HTTPServerResponse& response)
{
	Logger& logger = Logger::get("Network");

	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string fragmentUrl = videoList.getFragmentUrl(episode_id_, bitrate_, type_, start_time_);

	if (fragmentUrl.empty())
	{
		response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
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
		logger.trace("Fetching fragment from remote server (%s)...", fragmentUrl);
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

		// Set a timeout for the request
		session.setTimeout(Timespan(REMOTE_TIMEOUT, 0));

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

		response.setStatusAndReason(responseStatus);

		for (const auto& [key, value] : fragmentResponse)
			response.set(key, value);

		std::ostream& responseBody = response.send();
		responseBody.write(bodyStr.data(), static_cast<long long>(bodyStr.size()));
	}
	catch (Poco::Exception& ex)
	{
		logger.error(
			"An exception occurred while fetching media fragment from the remote server! [episode_id: %s, bitrate: %s, type: %s, start_time: %s] (%s)",
			episode_id_,
			bitrate_,
			type_,
			start_time_,
			ex.displayText());
		response.setStatusAndReason(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		response.send();
	}
}
