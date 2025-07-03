#pragma once

class SubtitleOverride final : public Poco::Util::Subsystem
{
public:
	[[nodiscard]] const char* name() const override;

	std::string overrideSubtitles(const std::string& episode_id, const std::string& track_name, std::string& data_raw);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	std::map<std::string, std::map<std::string, std::vector<std::string>>> m_subtitle_overrides_;

	bool closed_captioning_ = false;
	bool music_notes_ = false;

	static std::string extractCaptionKey(const std::string& file_name);

	void parseJsonOverride(const std::string& path, const std::string& file_name, const std::string& episode_id,
	                       std::map<std::string, std::vector<std::string>>& overrides) const;
	void parseBsonOverride(const std::string& path, const std::string& file_name, const std::string& episode_id,
	                       std::map<std::string, std::vector<std::string>>& overrides) const;
};
