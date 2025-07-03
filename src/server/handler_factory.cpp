#include "pch.hpp"
#include "handler_factory.hpp"

#include "handlers/error.hpp"

using Poco::Logger;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;

HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
{
	return new ErrorHandler(HTTPResponse::HTTP_NOT_IMPLEMENTED);
}
