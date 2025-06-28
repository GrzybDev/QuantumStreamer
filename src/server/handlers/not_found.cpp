#include "pch.hpp"
#include "not_found.hpp"

using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

void NotFoundHandler::handleWithLogging(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
	response.send();
}
