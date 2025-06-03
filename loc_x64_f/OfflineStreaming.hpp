#pragma once

class OfflineStreaming : public Poco::Util::Subsystem
{
public:
	const char* name() const override;

	std::string GetLocalClientManifest(std::string episodeId);
	std::string GetLocalFragment(std::string episodeId, std::string trackName, std::string bitrate, std::string startTime);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	struct SmoothFragment
	{
		long long moofOffset;
		long long trafNumber;
		long long trunNumber;
		long long sampleNumber;
	};

	struct SmoothTrack
	{
		char version;
		int trackId;
		int lengthSizeOfTrafNum;
		int lengthSizeOfTrunNum;
		int lengthSizeOfSampleNum;
		std::map<std::string, SmoothFragment> fragments;
	};

	struct SmoothMedia
	{
		Poco::Path sourceFile;
		std::string systemBitrate;
		std::map<std::string, SmoothTrack> track;
	};

	struct SmoothStream
	{
		Poco::Path clientManifestRelativePath;
		std::map<std::string, SmoothMedia> mediaMap;
	};

	std::map<std::string, SmoothStream> _streams;

	void processMediaNodes(const std::string& tagName, Poco::XML::Document* doc, std::string episodeId, const Poco::Path& episodePath, SmoothStream& stream);
	std::pair<bool, SmoothTrack> preloadTrack(Poco::Path path);
};
