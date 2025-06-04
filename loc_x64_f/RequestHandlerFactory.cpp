#include "pch.hpp"
#include "RequestHandlerFactory.hpp"

#include "FragmentRequestHandler.hpp"
#include "ManifestRequestHandler.hpp"
#include "SubtitleFragmentRequestHandler.hpp"

using Poco::Logger;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;

HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
{
	Logger& logger = Logger::get("HTTP");

	logger.debug("%s %s %s", request.getMethod(), request.getURI(), request.getVersion());

	if (request.getMethod() == HTTPRequest::HTTP_GET)
	{
		const std::string& uri = request.getURI();

		std::regex manifestUrlPattern("^/([^/]+)/manifest$");
		std::regex captionFragmentUrlPattern(
			R"(^/([^/]+)/QualityLevels\((\d+)\)/Fragments\(([^=]+)_captions=(\d+)\)$)");
		std::regex fragmentUrlPattern(R"(^/([^/]+)/QualityLevels\((\d+)\)/Fragments\(([^=]+)=(\d+)\)$)");
		std::smatch match;

		if (std::regex_match(uri, match, manifestUrlPattern))
		{
			std::string episodeId = match[1].str();
			return new ManifestRequestHandler(episodeId);
		}

		if (std::regex_match(uri, match, captionFragmentUrlPattern))
		{
			std::string episodeId = match[1].str();
			std::string qualityLevel = match[2].str();
			std::string langCode = match[3].str();
			std::string startTime = match[4].str();

			return new SubtitleFragmentRequestHandler(episodeId, qualityLevel, langCode, startTime);
		}

		if (std::regex_match(uri, match, fragmentUrlPattern))
		{
			std::string episodeId = match[1].str();
			std::string qualityLevel = match[2].str();
			std::string fragmentType = match[3].str();
			std::string startTime = match[4].str();

			return new FragmentRequestHandler(episodeId, qualityLevel, fragmentType, startTime);
		}
	}


	return nullptr;
}
