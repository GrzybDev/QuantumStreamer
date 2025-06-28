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
