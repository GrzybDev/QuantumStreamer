#include "pch.hpp"
#include "handler_factory.hpp"

#include "handlers/not_found.hpp"
#include "handlers/not_implemented.hpp"

using Poco::Logger;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;

HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
{
	if (request.getMethod() == HTTPRequest::HTTP_GET)
	{
		return new NotFoundHandler();
	}

	return new NotImplementedHandler();
}
