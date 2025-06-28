#pragma once

class FragmentRequestHandler final : public BaseHandler
{
public:
	explicit FragmentRequestHandler(std::string episode_id, std::string bitrate, std::string type,
	                                std::string start_time);
	void handleWithLogging(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

private:
	std::string episode_id_;
	std::string bitrate_;
	std::string type_;
	std::string start_time_;
	std::string text_lang_code_;
	bool is_text_stream_;

	[[nodiscard]] std::string processSubtitleData(const std::string& data) const;
};
