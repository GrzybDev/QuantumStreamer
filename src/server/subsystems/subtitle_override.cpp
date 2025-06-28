#include "pch.hpp"
#include "subtitle_override.hpp"

#include "video_list.hpp"

using Poco::AutoPtr;
using Poco::BinaryReader;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Logger;
using Poco::Path;
using Poco::Dynamic::Var;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::MongoDB::Document;
using Poco::Util::Application;
using Poco::XML::DOMWriter;
using Poco::XML::Element;
using Poco::XML::InputSource;
using Poco::XML::DOMParser;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::XMLWriter;
using json_array = Poco::JSON::Array;
using bson_array = Poco::MongoDB::Array;

const char* SubtitleOverride::name() const
{
	return "SubtitleOverride";
}

void SubtitleOverride::initialize(Application& app)
{
	Logger& logger = Logger::get(name());
	logger.debug("Loading subtitle overrides...");

	const std::string episodesPath = app.config().getString("Server.EpisodesPath", "./videos/episodes");
	VideoList& videoList = app.getSubsystem<VideoList>();

	// Check if the episodes path exists
	for (const auto episodes = videoList.getEpisodeList(); const auto& episodeId : episodes)
	{
		File episodeDir(episodesPath + "/" + episodeId);
		if (!(episodeDir.exists() && episodeDir.isDirectory())) continue;

		std::map<std::string, std::vector<std::string>> overrides;

		for (DirectoryIterator it(episodeDir), end; it != end; ++it)
		{
			const auto& filePath = it.path();
			const std::string& fileName = filePath.getFileName();
			std::string extension = Path(fileName).getExtension();

			if (fileName.find("_captions_override") == std::string::npos) continue;

			if (extension == "json")
				parseJsonOverride(filePath.toString(), fileName, episodeId, overrides);
			else if (extension == "bson")
				parseBsonOverride(filePath.toString(), fileName, episodeId, overrides);
		}

		if (overrides.empty())
		{
			logger.warning("No subtitle overrides found for episode %s!", episodeId);
			continue;
		}

		m_subtitle_overrides_[episodeId] = std::move(overrides);
	}

	logger.information("Successfully loaded caption overrides for %s episodes!",
	                   std::to_string(m_subtitle_overrides_.size()));
}

void SubtitleOverride::uninitialize()
{
	m_subtitle_overrides_.clear();
}

std::string SubtitleOverride::extractCaptionKey(const std::string& file_name)
{
	const std::string baseName = Path(file_name).getFileName();
	const auto underscorePos = baseName.find_last_of('_');
	const auto dotPos = baseName.find_last_of('.');
	return baseName.substr(0, std::min(underscorePos, dotPos));
}

void SubtitleOverride::parseJsonOverride(const std::string& path, const std::string& file_name,
                                         const std::string& episode_id,
                                         std::map<std::string, std::vector<std::string>>& overrides) const
{
	Logger& logger = Logger::get(name());

	std::ifstream file(path);
	if (!file)
	{
		logger.error("Failed to open subtitle override file: %s", path);
		return;
	}

	Parser parser;
	const Var result = parser.parse(file);
	const auto& jsonObject = result.extract<Object::Ptr>();
	file.close();

	json_array::Ptr segmentsArray = jsonObject->getArray("segments");
	if (segmentsArray.isNull())
	{
		logger.warning("No segments found in subtitle override file: %s", path);
		return;
	}

	std::vector<std::string> segments;
	for (size_t i = 0; i < segmentsArray->size(); ++i)
		segments.push_back(segmentsArray->getElement<std::string>(static_cast<unsigned int>(i)));

	std::string captionKey = extractCaptionKey(file_name);
	overrides[captionKey] = std::move(segments);

	logger.debug("Loaded %s caption overrides from JSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode_id);
}

void SubtitleOverride::parseBsonOverride(const std::string& path, const std::string& file_name,
                                         const std::string& episode_id,
                                         std::map<std::string, std::vector<std::string>>& overrides) const
{
	Logger& logger = Logger::get(name());

	std::string captionKey = extractCaptionKey(file_name);
	if (overrides.contains(captionKey))
	{
		logger.debug("Skipping BSON override for %s (already loaded from JSON)", captionKey);
		return;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		logger.error("Failed to open BSON subtitle override file: %s", path);
		return;
	}

	BinaryReader reader(file);
	Document document;
	document.read(reader);
	file.close();

	std::vector<std::string> segments;
	if (document.isType<bson_array::Ptr>("segments"))
	{
		auto& array = document.get<bson_array::Ptr>("segments");
		for (size_t i = 0; i < array->size(); ++i)
			segments.push_back(array->get<std::string>(i));
	}

	overrides[captionKey] = std::move(segments);

	logger.debug("Loaded %s caption overrides from GSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode_id);
}
