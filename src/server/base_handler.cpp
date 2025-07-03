#include "pch.hpp"
#include "base_handler.hpp"

void BaseHandler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
	// let derived class do its work
	handleWithLogging(request, response);

	// after response is finished, log status and content length
	Poco::Logger& logger = Poco::Logger::get("Network");

	const auto contentLength = response.getContentLength();
	std::string contentLengthStr = (contentLength >= 0)
		                               ? std::to_string(contentLength)
		                               : "-";

	logger.debug("%s - \"%s %s %s\" %d %s",
	             request.clientAddress().toString(),
	             request.getMethod(),
	             request.getURI(),
	             request.getVersion(),
	             static_cast<int>(response.getStatus()),
	             contentLengthStr);
}
