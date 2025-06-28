#pragma once

#include "../base_handler.hpp"

class NotImplementedHandler final : public BaseHandler
{
public:
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;
};
