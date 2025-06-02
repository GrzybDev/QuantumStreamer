#pragma once

class SubtitleOverride : public Poco::Util::Subsystem
{
public:
	const char* name() const override;

	std::string OverrideSubtitles(std::string episodeId, std::string trackName, std::string& dataRaw,
	                              bool appendEpTitle);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	std::map<std::string, std::map<std::string, std::vector<std::string>>> m_subtitleOverrides;
	std::map<std::string, std::string> m_episodeNames;

	bool _closedCaptioning;
	bool _musicNotes;
	bool _episodeNames;
};
