#include "pch.hpp"
#include "handler_factory.hpp"

#include "handlers/manifest.hpp"
#include "handlers/error.hpp"

using Poco::Logger;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;

HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
{
	if (request.getMethod() == HTTPRequest::HTTP_GET)
	{
		const std::string& uri = request.getURI();

		const std::regex manifestUrlPattern(R"(^/([^/]+)/manifest$)");
		std::smatch match;

		if (std::regex_match(uri, match, manifestUrlPattern))
		{
			const std::string episodeId = match[1].str();
			return new ManifestRequestHandler(episodeId);
		}

		return new ErrorHandler(HTTPResponse::HTTP_NOT_FOUND);
	}

	return new ErrorHandler(HTTPResponse::HTTP_NOT_IMPLEMENTED);
}
