#pragma once

#include <memory>
#include <string>

#include <Poco/Net/HTTPRequestHandlerFactory.h>

#include "content_sources.hpp"

namespace qs
{
	/// Routes the game's manifest and fragment requests to their handlers.
	class RequestHandlerFactory final : public Poco::Net::HTTPRequestHandlerFactory
	{
	public:
		/// @param services What the handlers serve episodes from.
		explicit RequestHandlerFactory(const Services& services);

		Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override;

	private:
		[[nodiscard]] std::unique_ptr<ContentSource> matchContent(const std::string& uri) const;

		Services services_;
	};
}
