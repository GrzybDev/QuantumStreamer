#pragma once

class ManifestRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	ManifestRequestHandler(std::string episodeId);
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	std::string _episodeId;
};
