#include "pch.hpp"
#include "not_implemented.hpp"

using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

void NotImplementedHandler::handleWithLogging(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_NOT_IMPLEMENTED);
	response.send();
}
