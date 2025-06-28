#include "pch.hpp"
#include "error.hpp"

using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

ErrorHandler::ErrorHandler(const HTTPResponse::HTTPStatus status) : status_(status)
{
}

void ErrorHandler::handleWithLogging(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatusAndReason(status_);
	response.send();
}
