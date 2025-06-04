#pragma once

class SubtitleOverride : public Poco::Util::Subsystem
{
public:
	const char* name() const override;

	std::string OverrideSubtitles(const std::string& episodeId, const std::string& trackName, std::string& dataRaw,
	                              bool appendEpTitle);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	std::map<std::string, std::map<std::string, std::vector<std::string>>> m_subtitleOverrides;
	std::map<std::string, std::string> m_episodeNames;

	bool _closedCaptioning = false;
	bool _musicNotes = false;
	bool _episodeNames = false;

	static std::string extractCaptionKey(const std::string& fileName);
	void parseJsonOverride(const std::string& path, const std::string& fileName, const std::string& episode,
	                       std::map<std::string, std::vector<std::string>>& overrides);
	void parseBsonOverride(const std::string& path, const std::string& fileName, const std::string& episode,
	                       std::map<std::string, std::vector<std::string>>& overrides);
};
