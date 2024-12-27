#pragma once

class SmoothStreaming
{
public:
	SmoothStreaming();

	static SmoothStreaming& GetInstance()
	{
		static SmoothStreaming* instance;

		if (instance == nullptr)
			instance = new SmoothStreaming();

		return *instance;
	}

	std::string GetClientManifest(std::string episodeId);
	std::string GetFragment(std::string episodeId, std::string trackName, long long bitrate, long long time);

private:
	enum MediaType
	{
		Audio,
		Video,
		Text
	};

	struct SmoothFragment
	{
		long long time;
		long long moofOffset;
		long long trafNumber;
		long long trunNumber;
		long long sampleNumber;
	};

	struct SmoothStream
	{
		bool success = false;
		char version;
		int trackId;
		int lengthSizeOfTrafNum;
		int lengthSizeOfTrunNum;
		int lengthSizeOfSampleNum;
		std::list<SmoothFragment> fragments;
	};

	struct SmoothMedia
	{
		MediaType type;
		std::string name;
		std::string src;
		unsigned int systemBitrate;
		SmoothStream stream;
	};

	const boost::filesystem::path episodes_path = "videos/episodes/";

	std::map<std::string, std::string> clientManifestPaths;
	std::map<std::string, std::list<SmoothMedia>> smoothStreams;

	SmoothStream PreloadStream(std::string filePath);
};
