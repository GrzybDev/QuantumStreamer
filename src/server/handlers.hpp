#pragma once

#include <memory>
#include <string>

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPResponse.h>

#include "content_sources.hpp"

namespace qs
{
	/// Writes an access log line after every response and answers 500 when the handler fails.
	class LoggedRequestHandler : public Poco::Net::HTTPRequestHandler
	{
	public:
		void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) final;

	protected:
		/// Answers the request.
		/// @param request The game's request.
		/// @param response The response to send.
		virtual void respond(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) = 0;
	};

	/// Answers with a bare status code.
	class StatusHandler final : public LoggedRequestHandler
	{
	public:
		/// @param status The status to answer with.
		explicit StatusHandler(Poco::Net::HTTPResponse::HTTPStatus status);

	protected:
		void respond(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

	private:
		Poco::Net::HTTPResponse::HTTPStatus status_;
	};

	/// Serves a content source from disk or, unless offline, by forwarding the request to the CDN.
	class ContentHandler final : public LoggedRequestHandler
	{
	public:
		/// @param source What is requested.
		/// @param offlineMode Whether the CDN must not be used.
		ContentHandler(std::unique_ptr<ContentSource> source, bool offlineMode);

	protected:
		void respond(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

	private:
		void forward(const std::string& url, const Poco::Net::HTTPServerRequest& request,
		             Poco::Net::HTTPServerResponse& response) const;

		std::unique_ptr<ContentSource> source_;
		bool offlineMode_;
	};
}
