#include "pch.hpp"
#include "FragmentRequestHandler.hpp"

#include "VideoList.hpp"

using Poco::Util::Application;

FragmentRequestHandler::FragmentRequestHandler(std::string episodeId, std::string bitrate, std::string type,
                                               std::string startTime) : _episodeId(std::move(episodeId)),
                                                                        _bitrate(std::move(bitrate)),
                                                                        _type(std::move(type)),
                                                                        _startTime(std::move(startTime))
{
}

void FragmentRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& request,
                                           Poco::Net::HTTPServerResponse& response)
{
	Application& app = Application::instance();
	VideoList& videoList = app.getSubsystem<VideoList>();

	std::string manifestUrl = videoList.getFragmentUrl(_episodeId, _bitrate, _type, _startTime);

	if (manifestUrl.empty())
	{
		response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
		response.send();
		return;
	}

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

	response.setStatus(manifestResponse.getStatus());

	for (const auto& header : manifestResponse)
	{
		response.set(header.first, header.second);
	}

	std::ostream& responseBody = response.send();
	responseBody.write(bodyStr.data(), bodyStr.size());
}
