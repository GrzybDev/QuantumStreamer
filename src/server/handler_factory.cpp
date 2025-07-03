#include "pch.hpp"
#include "handler_factory.hpp"

#include "handlers/fragment.hpp"
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
		const std::regex fragmentUrlPattern(R"(^/([^/]+)/QualityLevels\((\d+)\)/Fragments\(([^=]+)=(\d+)\)$)");

		std::smatch match;

		if (std::regex_match(uri, match, manifestUrlPattern))
		{
			const std::string episodeId = match[1].str();
			return new ManifestRequestHandler(episodeId);
		}

		if (std::regex_match(uri, match, fragmentUrlPattern))
		{
			std::string episodeId = match[1].str();
			std::string qualityLevel = match[2].str();
			std::string fragmentType = match[3].str();
			std::string startTime = match[4].str();

			return new FragmentRequestHandler(episodeId, qualityLevel, fragmentType, startTime);
		}

		return new ErrorHandler(HTTPResponse::HTTP_NOT_FOUND);
	}

	return new ErrorHandler(HTTPResponse::HTTP_NOT_IMPLEMENTED);
}
