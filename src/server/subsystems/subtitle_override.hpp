#pragma once

class SubtitleOverride final : public Poco::Util::Subsystem
{
public:
	[[nodiscard]] const char* name() const override;

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	std::map<std::string, std::map<std::string, std::vector<std::string>>> m_subtitle_overrides_;

	static std::string extractCaptionKey(const std::string& file_name);

	void parseJsonOverride(const std::string& path, const std::string& file_name, const std::string& episode_id,
	                       std::map<std::string, std::vector<std::string>>& overrides) const;
	void parseBsonOverride(const std::string& path, const std::string& file_name, const std::string& episode_id,
	                       std::map<std::string, std::vector<std::string>>& overrides) const;
};
