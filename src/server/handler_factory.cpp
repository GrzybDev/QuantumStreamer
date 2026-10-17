#include "pch.hpp"
#include "handler_factory.hpp"

#include "handlers.hpp"

using Poco::Net::HTTPResponse;

namespace qs
{
	RequestHandlerFactory::RequestHandlerFactory(const Services& services) : services_(services)
	{
	}

	Poco::Net::HTTPRequestHandler* RequestHandlerFactory::createRequestHandler(
		const Poco::Net::HTTPServerRequest& request)
	{
		if (request.getMethod() != Poco::Net::HTTPRequest::HTTP_GET)
			return new StatusHandler(HTTPResponse::HTTP_NOT_IMPLEMENTED);

		if (std::unique_ptr<ContentSource> source = matchContent(request.getURI()))
			return new ContentHandler(std::move(source), services_.offlineMode);

		return new StatusHandler(HTTPResponse::HTTP_NOT_FOUND);
	}

	std::unique_ptr<ContentSource> RequestHandlerFactory::matchContent(const std::string& uri) const
	{
		static const std::regex manifest(R"(^/([^/]+)/manifest$)");
		static const std::regex fragment(R"(^/([^/]+)/QualityLevels\((\d+)\)/Fragments\(([^=]+)=(\d+)\)$)");

		std::smatch match;
		if (std::regex_match(uri, match, manifest))
			return std::make_unique<ManifestSource>(services_, match[1]);

		if (std::regex_match(uri, match, fragment))
			return std::make_unique<FragmentSource>(services_, FragmentRequest{match[1], match[2], match[3], match[4]});

		return nullptr;
	}
}
