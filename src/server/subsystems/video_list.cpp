#include "pch.hpp"
#include "video_list.hpp"

using Poco::File;
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
	if (!File(videoListPath).exists())
	{
		logger.error("Video list file does not exist: %s, no episodes can be loaded!", videoListPath);
		video_list_ = new Object;
		return;
	}

	video_list_ = loadVideoList(videoListPath);

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

Object::Ptr VideoList::loadVideoList(const std::string& path) const
{
	Logger& logger = Logger::get(name());
	logger.debug("Loading video list (from %s)...", path);

	std::ifstream videoListStream(path, std::ios::binary);

	if (!videoListStream.is_open())
	{
		logger.warning("Failed to open video list file: %s", path);
		return new Object;
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

	return result.extract<Object::Ptr>();
}


void VideoList::patch(unsigned short port)
{
	Logger& logger = Logger::get(name());
	logger.debug("Patching video list file...");

	const Application& app = Application::instance();
	const std::string videoListPath = app.config().getString("Server.VideoListPath", "./data/videoList_original.rmdj");
	const std::string gameVideoListPath = "./data/videoList.rmdj";

	if (!File(videoListPath).exists())
	{
		if (File(gameVideoListPath).exists())
		{
			logger.warning("Copying video list file from %s to %s...", gameVideoListPath, videoListPath);
			File(gameVideoListPath).copyTo(videoListPath);
		}
		else
		{
			logger.error("No video list file found to patch: %s", videoListPath);
			return;
		}
	}

	video_list_ = loadVideoList(videoListPath);

	// Build the patched video list
	// Each episode id is a key, and the value is in format:
	// http://127.0.0.1:<port>/<episode_id>/manifest

	// Create a new JSON object for the patched video list
	Object::Ptr patchedVideoList = new Object;

	for (const auto& episodeId : getEpisodeList())
	{
		const std::string patchedUrl = std::format("http://127.0.0.1:{}/{}/manifest", port, episodeId);
		patchedVideoList->set(episodeId, patchedUrl);
	}

	// Save the patched video list to the std::string
	std::ostringstream oss;
	Poco::JSON::Stringifier::stringify(patchedVideoList, oss, 4);

	std::string patchedVideoListStr = oss.str();

	// Encrypt the patched video list
	for (size_t i = 0; i < patchedVideoListStr.size(); ++i)
		patchedVideoListStr[i] ^= RMDJ_ENCRYPTION_KEY[i % 32];

	std::ofstream outFile(gameVideoListPath, std::ios::binary | std::ios::trunc);
	logger.debug("Writing patched video list to %s...", gameVideoListPath);

	if (!outFile.is_open())
	{
		logger.error("Failed to open video list file for writing: %s", videoListPath);
		return;
	}

	outFile.write(patchedVideoListStr.data(), patchedVideoListStr.size());
	outFile.close();

	logger.information("Video list file patched successfully! (Videos count: %d)",
	                   static_cast<int>(patchedVideoList->size()));
}
