#pragma once

class RequestHandlerFactory final : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override;
};
