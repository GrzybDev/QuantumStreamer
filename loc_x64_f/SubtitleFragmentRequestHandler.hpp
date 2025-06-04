#pragma once

class SubtitleFragmentRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	SubtitleFragmentRequestHandler(std::string episodeId, std::string bitrate, std::string langCode,
	                               std::string startTime);
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	std::string _episodeId;
	std::string _bitrate;
	std::string _langCode;
	std::string _startTime;

	std::string processSubtitleData(const std::string& data) const;
};
