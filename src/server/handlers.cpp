#include "pch.hpp"
#include "handlers.hpp"

#include "app/logging.hpp"

using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

namespace qs
{
	namespace
	{
		// The game retries a request after about this long.
		constexpr long kRemoteTimeoutSeconds = 20;

		Poco::Logger& logger()
		{
			return Poco::Logger::get(channel::Network);
		}

		// These headers describe one connection, so they are not forwarded; the length is set for the new body.
		bool isConnectionHeader(const std::string& name)
		{
			static constexpr std::array kHeaders{
				"Connection", "Content-Length", "Host", "Keep-Alive", "Proxy-Authenticate", "Proxy-Authorization", "TE",
				"Trailer", "Transfer-Encoding", "Upgrade"
			};

			return std::ranges::any_of(kHeaders, [&](const char* header) { return Poco::icompare(name, header) == 0; });
		}

		void sendBody(HTTPServerResponse& response, const std::string_view body)
		{
			response.setContentLength64(static_cast<Poco::Int64>(body.size()));
			response.send().write(body.data(), static_cast<std::streamsize>(body.size()));
		}

		void sendStatus(HTTPServerResponse& response, const HTTPResponse::HTTPStatus status)
		{
			response.setStatusAndReason(status);
			sendBody(response, {});
		}
	}

	void LoggedRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		try
		{
			respond(request, response);
		}
		catch (const std::exception& error)
		{
			const auto* pocoError = dynamic_cast<const Poco::Exception*>(&error);
			logger().error(std::format("Failed to answer \"{} {}\" ({})", request.getMethod(), request.getURI(),
			                           pocoError ? pocoError->displayText() : error.what()));

			if (!response.sent())
				sendStatus(response, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
		}

		if (logger().debug())
		{
			const Poco::Int64 length = response.getContentLength64();
			logger().debug(std::format("{} - \"{} {} {}\" {} {}", request.clientAddress().toString(),
			                           request.getMethod(), request.getURI(), request.getVersion(),
			                           static_cast<int>(response.getStatus()),
			                           length >= 0 ? std::to_string(length) : "-"));
		}
	}

	StatusHandler::StatusHandler(const HTTPResponse::HTTPStatus status) : status_(status)
	{
	}

	void StatusHandler::respond(HTTPServerRequest& /*request*/, HTTPServerResponse& response)
	{
		sendStatus(response, status_);
	}

	ContentHandler::ContentHandler(std::unique_ptr<ContentSource> source, const bool offlineMode) :
		source_(std::move(source)),
		offlineMode_(offlineMode)
	{
	}

	void ContentHandler::respond(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		const std::optional<std::string> url = source_->remoteUrl();
		if (!url)
		{
			sendStatus(response, HTTPResponse::HTTP_NOT_FOUND);
			return;
		}

		if (std::optional<std::string> local = source_->loadLocal())
		{
			if (logger().trace())
				logger().trace(std::format("Serving the local {}...", source_->describe()));

			sendBody(response, source_->transform(std::move(*local)));
			return;
		}

		if (offlineMode_)
		{
			logger().warning(std::format("Offline mode is enabled, but the requested {} is not available locally",
			                             source_->describe()));
			sendStatus(response, HTTPResponse::HTTP_NOT_ACCEPTABLE);
			return;
		}

		forward(*url, request, response);
	}

	void ContentHandler::forward(const std::string& url, const HTTPServerRequest& request,
	                             HTTPServerResponse& response) const
	{
		const Poco::URI uri(url);

		Poco::Net::HTTPRequest remoteRequest(Poco::Net::HTTPRequest::HTTP_GET, url, Poco::Net::HTTPMessage::HTTP_1_1);
		for (const auto& [name, value] : request)
			if (!isConnectionHeader(name))
				remoteRequest.add(name, value);
		remoteRequest.setHost(uri.getHost());

		if (logger().trace())
			logger().trace(std::format("Fetching the {} from the remote server ({})...", source_->describe(), url));

		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setTimeout(Poco::Timespan(kRemoteTimeoutSeconds, 0));
		session.sendRequest(remoteRequest);

		HTTPResponse remoteResponse;
		std::string body;
		Poco::StreamCopier::copyToString(session.receiveResponse(remoteResponse), body);

		if (remoteResponse.getStatus() == HTTPResponse::HTTP_OK)
			body = source_->transform(std::move(body));
		else
		{
			logger().error(std::format("Failed to fetch the {}, the remote server answered {}",
			                           source_->describe(), static_cast<int>(remoteResponse.getStatus())));
			if (logger().trace())
				logger().trace(std::format("Remote server response: {}", body));
		}

		response.setStatusAndReason(remoteResponse.getStatus());
		for (const auto& [name, value] : remoteResponse)
			if (!isConnectionHeader(name))
				response.add(name, value);

		sendBody(response, body);
	}
}
