#pragma once

class SubtitleOverride final : public Poco::Util::Subsystem
{
public:
	[[nodiscard]] const char* name() const override;

	std::string overrideSubtitles(const std::string& episode_id, const std::string& track_name, std::string& data_raw,
	                              const std::string& start_time);

	void load();

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	struct SrtSegment
	{
		double begin_time_sec;
		double end_time_sec;
		std::string text;
	};

	std::map<std::string, std::map<std::string, std::vector<SrtSegment>>> m_subtitle_overrides_;

	bool closed_captioning_ = false;
	bool music_notes_ = false;

	static std::string extractCaptionKey(const std::string& file_name);

	void parseSrtOverride(const std::string& path, const std::string& file_name, const std::string& episode_id,
	                      std::map<std::string, std::vector<SrtSegment>>& overrides);
	static double parseSrtTime(const std::string& time_str);
	static double parseTtmlTime(const std::string& time_str);
	static std::string formatTtmlTime(double time_sec);
};
