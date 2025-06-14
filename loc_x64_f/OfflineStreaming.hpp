#pragma once

class OfflineStreaming : public Poco::Util::Subsystem
{
public:
	const char* name() const override;

	std::string GetLocalClientManifest(const std::string& episodeId);
	std::string GetLocalFragment(const std::string& episodeId, const std::string& trackName, const std::string& bitrate,
	                             const std::string& startTime);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	struct SmoothFragment
	{
		unsigned long long moofOffset;
		unsigned long long trafNumber;
		unsigned long long trunNumber;
		unsigned long long sampleNumber;
	};

	struct SmoothTrack
	{
		char version;
		unsigned int trackId;
		int lengthSizeOfTrafNum;
		int lengthSizeOfTrunNum;
		int lengthSizeOfSampleNum;
		std::map<std::string, SmoothFragment> fragments;
	};

	struct SmoothMedia
	{
		Poco::Path sourceFile;
		std::string systemBitrate;
		SmoothTrack track;
	};

	struct SmoothStream
	{
		Poco::Path clientManifestRelativePath;
		std::map<std::string, SmoothMedia> mediaMap;
	};

	std::map<std::string, SmoothStream> _streams;

	void processMediaNodes(const std::string& tagName, Poco::XML::Document* doc, const std::string& episodeId,
	                       const std::string& episodePath, SmoothStream& stream);
	std::pair<bool, SmoothTrack> preloadTrack(const std::string& path);
};
