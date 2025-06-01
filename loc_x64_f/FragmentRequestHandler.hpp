#pragma once

class FragmentRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	FragmentRequestHandler(std::string episodeId, std::string bitrate, std::string type, std::string startTime);
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	std::string _episodeId;
	std::string _bitrate;
	std::string _type;
	std::string _startTime;
};
