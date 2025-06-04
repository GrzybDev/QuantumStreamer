#include "pch.hpp"
#include "VideoList.hpp"

using Poco::Logger;
using Poco::Dynamic::Var;
using Poco::JSON::Object;
using Poco::JSON::Parser;
using Poco::Util::Application;

const char* VideoList::name() const
{
	return "VideoList";
}

void VideoList::initialize(Application& app)
{
	Logger& logger = Logger::get("Server");

	std::string videoListPath = app.config().getString("Server.VideoListPath", "./data/videoList_original.rmdj");
	logger.debug("Loading video list (from %s)...", videoListPath);

	std::ifstream videoListStream(videoListPath, std::ios::binary);

	if (!videoListStream.is_open())
	{
		logger.warning("Failed to open video list file: %s", videoListPath);
		return;
	}

	// Read all the bytes from the file
	std::vector videoListData((std::istreambuf_iterator(videoListStream)), std::istreambuf_iterator<char>());
	videoListStream.close();

	// Decrypt the video list data
	for (size_t i = 0; i < videoListData.size(); ++i)
		videoListData[i] ^= RMDJEncryptionKey[i % 32]; // NOLINT(cppcoreguidelines-narrowing-conversions)

	auto tempVideoListStr = std::string(videoListData.begin(), videoListData.end());

	Parser parser;
	Var result = parser.parse(tempVideoListStr);

	videoList = result.extract<Object::Ptr>();

	logger.information("Successfully loaded videos list! (Videos count: %d)", static_cast<int>(videoList->size()));
}

void VideoList::uninitialize()
{
	videoList->clear();
}

std::vector<std::string> VideoList::getEpisodeList()
{
	std::vector<std::string> episodes;
	videoList->getNames(episodes);
	return episodes;
}

std::string VideoList::getManifestUrl(const std::string& episodeId)
{
	if (!videoList->has(episodeId))
		return {};

	return videoList->getValue<std::string>(episodeId);
}

std::string VideoList::getFragmentUrl(const std::string& episodeId, const std::string& bitrate, const std::string& type,
                                      const std::string& startTime)
{
	if (!videoList->has(episodeId))
		return {};

	auto manifestUrl = videoList->getValue<std::string>(episodeId);

	// Replace "manifest" with QualityLevels({bitrate})/Fragments({type}={startTime})
	std::string fragmentUrl = manifestUrl;
	size_t pos = fragmentUrl.find("manifest");
	if (pos != std::string::npos)
		fragmentUrl.replace(pos, 8, "QualityLevels(" + bitrate + ")/Fragments(" + type + "=" + startTime + ")");
	else
	{
		Logger& logger = Logger::get("Server");
		logger.warning("Failed to find 'manifest' in URL: %s", manifestUrl);
		return {};
	}

	return fragmentUrl;
}
