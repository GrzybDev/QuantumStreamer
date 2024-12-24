#include "pch.hpp"

SubtitleOverride::SubtitleOverride()
{
	enableCC = boost::filesystem::exists("enableCC");

	const auto& videoList = VideoList::GetInstance();

	for (const auto& episode : videoList.episodeManifests)
		LoadOverrides(episode.first);
};

void SubtitleOverride::LoadOverrides(std::string episode)
{
	// Load all subtitle overrides for the episode
	// The path is as follows: videos/episodes/<episode>/<subtitle_name>_override.json
	const boost::filesystem::path path("videos/episodes/" + episode + "/");

	// Check if the directory exists and is indeed a directory
	if (exists(path) && is_directory(path))
	{
		// Iterate through the directory entries
		for (const auto& entry : boost::filesystem::directory_iterator(path))
		{
			// Check if the entry is a regular file, filename ends with _override and has a .json extension
			if (is_regular_file(entry) && entry.path().filename().string().find("_override") != std::string::npos &&
				entry.path().extension().string() == ".json")
			{
				try
				{
					boost::property_tree::ptree subtitleOverride;
					read_json(entry.path().string(), subtitleOverride);

					std::map<std::string, std::string> subtitleOverrideMap;
					for (const auto& subtitleOverrideEntry : subtitleOverride)
						subtitleOverrideMap[subtitleOverrideEntry.first] = subtitleOverrideEntry.second.get_value<
							std::string>();

					if (subtitleOverrides.find(episode) == subtitleOverrides.end())
						subtitleOverrides[episode] = std::map<std::string, std::map<std::string, std::string>>();

					// Get the subtitle name from the filename without the _override
					auto subtitleName = entry.path().stem().string();
					// enus_captions_override.json -> enus_captions_override
					subtitleName.erase(subtitleName.find("_override"), 9); // enus_captions_override -> enus_captions
					subtitleOverrides[episode][subtitleName] = subtitleOverrideMap;
				}
				catch (const boost::property_tree::json_parser_error& e)
				{
					BOOST_LOG_TRIVIAL(error) << "Failed to parse subtitle override: " << e.what();
				}
				catch (const std::exception& e)
				{
					BOOST_LOG_TRIVIAL(error) << "An error occurred while reading the subtitle override: " << e.what();
				}
				catch (...)
				{
					BOOST_LOG_TRIVIAL(error) << "An unknown error occurred while reading the subtitle override";
				}
			}
		}
	}
}

std::string SubtitleOverride::GetSubtitleOverride(std::string episode, std::string subtitleName,
                                                  std::string& originalChunk)
{
	// Check if the episode has any subtitle overrides
	if (subtitleOverrides.find(episode) != subtitleOverrides.end())
	{
		// Check if the subtitle name has any overrides
		if (subtitleOverrides[episode].find(subtitleName) != subtitleOverrides[episode].end())
		{
			// Read from 88 byte to the end of the file
			std::string subtitleXML = originalChunk;
			subtitleXML.erase(0, 0x58);

			pugi::xml_document subtitleData;
			pugi::xml_parse_result success = subtitleData.load_string(subtitleXML.c_str());
			bool isModified = false;

			if (success)
			{
				for (pugi::xml_node div : subtitleData.child("tt").child("body").children("div"))
				{
					for (pugi::xml_node p : div.children("p"))
					{
						std::string subtitle_id = p.attribute("xml:id").as_string();
						std::string subtitle_text = p.child("span").text().as_string();

						if (subtitleOverrides[episode][subtitleName].find(subtitle_id) != subtitleOverrides[episode][
							subtitleName].end())
						{
							std::string overrideText = subtitleOverrides[episode][subtitleName][subtitle_id];

							if (!enableCC)
							{
								boost::regex ccRegex("[ -]*\\[ .* \\]");
								overrideText = boost::regex_replace(overrideText, ccRegex, "");
							}

							p.child("span").text().set(overrideText.c_str());
							isModified = true;
						}
					}
				}
			}

			if (isModified)
			{
				std::ostringstream newSubtitleXMLStream;
				subtitleData.save(newSubtitleXMLStream);
				std::string newSubtitleXML = newSubtitleXMLStream.str();

				std::string newHeader = originalChunk.substr(0, 0x58);
				// size = 8 + replacedSubtitleData.size()
				unsigned int newSize = 8 + newSubtitleXML.size();

				for (int i = 0; i < 4; ++i)
				{
					newHeader[0x50 + i] = static_cast<unsigned char>((newSize >> (24 - i * 8)) & 0xFF);
				}

				return newHeader + newSubtitleXML;
			}
		}
	}

	// Return the original chunk if no override was found
	return originalChunk;
}
