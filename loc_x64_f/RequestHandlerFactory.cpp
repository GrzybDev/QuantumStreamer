#include "pch.hpp"
#include "RequestHandlerFactory.hpp"

Poco::Net::HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
	return nullptr;
}
