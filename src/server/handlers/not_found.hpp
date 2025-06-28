#pragma once

#include "../base_handler.hpp"

class NotFoundHandler final : public BaseHandler
{
public:
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;
};
