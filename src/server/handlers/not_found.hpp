#pragma once

class NotFoundHandler final : public BaseHandler
{
public:
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;
};
