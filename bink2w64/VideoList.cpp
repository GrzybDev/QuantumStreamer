#include "pch.hpp"

VideoList::VideoList()
{
	BOOST_LOG_FUNCTION();

	// Load the video list from the file
	ReadVideoList("data/videoList_original.rmdj");
}

void VideoList::ReadVideoList(std::string path)
{
	BOOST_LOG_TRIVIAL(info) << "Reading video list from: " << path;

	std::ifstream fileStream(path, std::ios::binary);
	std::string videoListString;

	if (!fileStream.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open video list file: " << path;
		videoListString = "{}";
	}
	else
	{
		// Read the video list bytes
		std::vector<char> videoListBytes((std::istreambuf_iterator<char>(fileStream)),
		                                 std::istreambuf_iterator<char>());
		fileStream.close();

		// Decrypt the video list bytes
		for (int i = 0; i < videoListBytes.size(); i++)
		{
			videoListBytes[i] ^= RMDJKey[i % 32];
		}

		videoListString = std::string(videoListBytes.begin(), videoListBytes.end());
	}

	try
	{
		boost::property_tree::ptree videoList;
		std::istringstream videoListStream(videoListString);
		read_json(videoListStream, videoList);

		for (const auto& video : videoList)
			episodeManifests[video.first] = video.second.get_value<std::string>();
	}
	catch (const boost::property_tree::json_parser_error& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to parse video list: " << e.what();
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "An error occurred while reading the video list: " << e.what();
	}
	catch (...)
	{
		BOOST_LOG_TRIVIAL(error) << "An unknown error occurred while reading the video list";
	}
}

std::string VideoList::GetEpisodeURL(std::string episode)
{
	auto it = episodeManifests.find(episode);
	if (it != episodeManifests.end())
		return it->second;
	return "";
}
