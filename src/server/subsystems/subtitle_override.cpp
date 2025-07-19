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
	if (!app.config().getBool("VideoList.PatchFile", true))
		load();
}

void SubtitleOverride::uninitialize()
{
	m_subtitle_overrides_.clear();
	m_episode_titles_.clear();
}

void SubtitleOverride::load()
{
	const Application& app = Application::instance();

	Logger& logger = Logger::get(name());
	logger.debug("Loading subtitle overrides...");

	closed_captioning_ = app.config().getBool("Subtitles.ClosedCaptioning", false);
	music_notes_ = app.config().getBool("Subtitles.MusicNotes", true);
	episode_titles_ = app.config().getBool("Subtitles.EpisodeTitles", true);

	logger.information("Closed captioning is %s", std::string(closed_captioning_ ? "enabled" : "disabled"));
	logger.information("Music notes are %s", std::string(music_notes_ ? "enabled" : "disabled"));
	logger.information("%s append episode titles to streams", std::string(episode_titles_ ? "Will" : "Will not"));

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

std::string SubtitleOverride::extractCaptionKey(const std::string& file_name)
{
	const std::string baseName = Path(file_name).getFileName();
	const auto underscorePos = baseName.find_last_of('_');
	const auto dotPos = baseName.find_last_of('.');
	return baseName.substr(0, std::min(underscorePos, dotPos));
}

void SubtitleOverride::parseJsonOverride(const std::string& path, const std::string& file_name,
                                         const std::string& episode_id,
                                         std::map<std::string, std::vector<std::string>>& overrides)
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

	if (jsonObject->has("episode_title"))
	{
		std::string trackName = file_name;
		trackName = trackName.substr(0, trackName.find("_override"));

		const std::string epTitleKey = episode_id + "_" + trackName;
		m_episode_titles_[epTitleKey] = jsonObject->getValue<std::string>("episode_title");
	}

	logger.debug("Loaded %s caption overrides from JSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode_id);
}

void SubtitleOverride::parseBsonOverride(const std::string& path, const std::string& file_name,
                                         const std::string& episode_id,
                                         std::map<std::string, std::vector<std::string>>& overrides)
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

	if (document.exists("episode_title"))
	{
		std::string trackName = file_name;
		trackName = trackName.substr(0, trackName.find("_override"));

		const std::string epTitleKey = episode_id + "_" + trackName;
		m_episode_titles_[epTitleKey] = document.get<std::string>("episode_title");
	}

	logger.debug("Loaded %s caption overrides from GSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode_id);
}

std::string SubtitleOverride::overrideSubtitles(const std::string& episode_id, const std::string& track_name,
                                                std::string& data_raw, bool is_episode_title_segment)
{
	Logger& logger = Logger::get(name());

	if (!m_subtitle_overrides_.contains(episode_id))
		return data_raw;

	auto& episodeOverrides = m_subtitle_overrides_[episode_id];

	if (!episodeOverrides.contains(track_name))
		return data_raw;

	auto& segments = episodeOverrides[track_name];

	std::istringstream xmlStream(data_raw);
	InputSource src(xmlStream);
	DOMParser parser;
	AutoPtr doc = parser.parse(&src);

	NodeList* pList = doc->getElementsByTagName("p");

	for (unsigned long i = 0; i < pList->length(); ++i)
	{
		auto pElem = dynamic_cast<Element*>(pList->item(i));
		auto span = pElem->getElementsByTagName("span")->item(0);

		std::string segmentIdStr = pElem->getAttribute("xml:id");

		std::string origText = span->innerText();
		std::string newText;

		if (int segmentId = std::stoi(segmentIdStr.substr(1)); segmentId >= 0 && segmentId < segments.size())
			newText = segments[segmentId];
		else
			newText = origText;

		if (!closed_captioning_)
		{
			// Remove closed captioning
			std::regex ccRegex(R"([ -]*\[ .* \])");
			newText = std::regex_replace(newText, ccRegex, "");
		}

		if (!music_notes_)
		{
			// Remove music notes
			std::string pattern;
			pattern += "\xE2\x99\xAA"; // UTF-8 encoding of music note (U+266A)
			pattern += "\xE2\x99\xAA";

			std::regex musicNoteRegex(pattern);
			newText = std::regex_replace(newText, musicNoteRegex, "");
		}

		while (Node* child = span->firstChild())
		{
			span->removeChild(child);
			child->release();
		}

		AutoPtr newSpanContent = doc->createTextNode(newText);
		span->appendChild(newSpanContent);

		logger.trace("Episode: %s (%s), Segment: %s ('%s' -> '%s')", episode_id, track_name, segmentIdStr, origText,
		             newText);
	}

	if (episode_titles_ && is_episode_title_segment)
	{
		if (std::string epTitleKey = episode_id + "_" + track_name; m_episode_titles_.contains(epTitleKey))
		{
			// Create new <p> element for episode title
			AutoPtr p = doc->createElement("p");
			p->setAttribute("xml:id", "episode_title");
			p->setAttribute("begin", "00:00:00.000");
			p->setAttribute("end", "00:00:01.850");
			p->setAttribute("region", "speaker");

			// Create <span> child with style and text content
			AutoPtr span = doc->createElement("span");
			span->setAttribute("style", "textStyle");

			// Create text node with episode title
			AutoPtr textNode = doc->createTextNode(m_episode_titles_[epTitleKey]);
			span->appendChild(textNode);

			// Append the <span> to the <p>
			p->appendChild(span);

			// Add the new <p> element to the first <div> in the document
			if (NodeList* divList = doc->getElementsByTagName("div"); divList->length() > 0)
			{
				auto firstDiv = dynamic_cast<Element*>(divList->item(0));
				firstDiv->appendChild(p);
				logger.trace("Added episode title '%s' to subtitles for episode %s", m_episode_titles_[epTitleKey],
				             episode_id);
			}
			else
			{
				logger.warning("No <div> found in subtitles for episode %s to add episode title!", episode_id);
			}
		}
	}

	DOMWriter writer;
	writer.setOptions(XMLWriter::WRITE_XML_DECLARATION | XMLWriter::CANONICAL_XML);

	std::ostringstream outputStream;
	writer.writeNode(outputStream, doc);

	return outputStream.str();
}
