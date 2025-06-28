#pragma once

class ErrorHandler final : public BaseHandler
{
public:
	explicit ErrorHandler(Poco::Net::HTTPResponse::HTTPStatus status);
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	Poco::Net::HTTPResponse::HTTPStatus status_;
};
