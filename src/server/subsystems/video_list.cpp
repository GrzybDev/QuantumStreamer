#include "pch.hpp"
#include "video_list.hpp"

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
	Logger& logger = Logger::get(name());

	const std::string videoListPath = app.config().getString("Server.VideoListPath", "./data/videoList_original.rmdj");

	// Check if the file exists
	if (!Poco::File(videoListPath).exists())
	{
		logger.error("Video list file does not exist: %s, no episodes can be loaded!", videoListPath);
		video_list_ = new Object;
		return;
	}

	logger.debug("Loading video list (from %s)...", videoListPath);

	std::ifstream videoListStream(videoListPath, std::ios::binary);

	if (!videoListStream.is_open())
	{
		logger.warning("Failed to open video list file: %s", videoListPath);
		video_list_ = new Object;
		return;
	}

	// Read all the bytes from the file
	std::vector videoListData((std::istreambuf_iterator(videoListStream)), std::istreambuf_iterator<char>());
	videoListStream.close();

	// Decrypt the video list data
	for (size_t i = 0; i < videoListData.size(); ++i)
		videoListData[i] ^= RMDJ_ENCRYPTION_KEY[i % 32];

	const auto tempVideoListStr = std::string(videoListData.begin(), videoListData.end());

	Parser parser;
	const Var result = parser.parse(tempVideoListStr);

	video_list_ = result.extract<Object::Ptr>();

	logger.information("Successfully loaded videos list! (Videos count: %d)", static_cast<int>(video_list_->size()));
}

void VideoList::uninitialize()
{
	video_list_->clear();
}

std::string VideoList::getManifestUrl(const std::string& episode_id)
{
	if (!video_list_->has(episode_id))
		return {};

	return video_list_->getValue<std::string>(episode_id);
}

std::string VideoList::getFragmentUrl(const std::string& episode_id, const std::string& bitrate,
                                      const std::string& type, const std::string& start_time)
{
	if (!video_list_->has(episode_id))
		return {};

	const auto manifestUrl = video_list_->getValue<std::string>(episode_id);

	// Replace "manifest" with QualityLevels({bitrate})/Fragments({type}={startTime})
	std::string fragmentUrl = manifestUrl;
	if (const size_t pos = fragmentUrl.find("manifest"); pos != std::string::npos)
		fragmentUrl.replace(pos, 8, "QualityLevels(" + bitrate + ")/Fragments(" + type + "=" + start_time + ")");
	else
	{
		Logger& logger = Logger::get("Server");
		logger.warning("Failed to find 'manifest' in URL: %s", manifestUrl);
		return {};
	}

	return fragmentUrl;
}

std::vector<std::string> VideoList::getEpisodeList()
{
	std::vector<std::string> episodes;
	video_list_->getNames(episodes);
	return episodes;
}
