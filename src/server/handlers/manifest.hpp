#pragma once

class ManifestRequestHandler final : public BaseHandler
{
public:
	explicit ManifestRequestHandler(std::string episode_id);
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	std::string episode_id_;
};
