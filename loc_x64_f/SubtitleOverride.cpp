#include "pch.hpp"
#include "SubtitleOverride.hpp"

#include "VideoList.hpp"

using Poco::AutoPtr;
using Poco::BinaryReader;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Logger;
using Poco::Path;
using Poco::Dynamic::Var;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::Util::Application;
using Poco::XML::DOMWriter;
using Poco::XML::Element;
using Poco::XML::InputSource;
using Poco::XML::DOMParser;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::XMLWriter;
using JSONArray = Poco::JSON::Array;
using MongoArray = Poco::MongoDB::Array;
using MongoDocument = Poco::MongoDB::Document;

const char* SubtitleOverride::name() const
{
	return "SubtitleOverride";
}

void SubtitleOverride::initialize(Application& app)
{
	Logger& logger = Logger::get("Server");
	logger.debug("Loading subtitle overrides...");

	_closedCaptioning = app.config().getBool("Subtitles.ClosedCaptioning", false);
	_musicNotes = app.config().getBool("Subtitles.MusicNotes", true);
	_episodeNames = app.config().getBool("Subtitles.EpisodeNames", true);

	logger.information("Closed captioning is %s", std::string(_closedCaptioning ? "enabled" : "disabled"));
	logger.information("Music notes are %s", std::string(_musicNotes ? "enabled" : "disabled"));
	logger.information("%s add episode titles to streams", std::string(_episodeNames ? "Will" : "Will NOT"));

	const std::string episodesPath = app.config().getString("Server.EpisodesPath", "./videos/episodes");
	VideoList& videoList = app.getSubsystem<VideoList>();

	// Check if the episodes path exists
	for (const auto episodes = videoList.getEpisodeList(); const auto& episode : episodes)
	{
		File episodeDir(episodesPath + "/" + episode);
		if (!(episodeDir.exists() && episodeDir.isDirectory())) continue;

		std::map<std::string, std::vector<std::string>> overrides;

		for (DirectoryIterator it(episodeDir), end; it != end; ++it)
		{
			const auto& filePath = it.path();
			const std::string& fileName = filePath.getFileName();
			std::string extension = Path(fileName).getExtension();

			if (fileName.find("_captions_override") == std::string::npos) continue;

			if (extension == "json")
				parseJsonOverride(filePath.toString(), fileName, episode, overrides);
			else if (extension == "bson")
				parseBsonOverride(filePath.toString(), fileName, episode, overrides);
		}

		if (overrides.empty())
		{
			logger.warning("No subtitle overrides found for episode %s!", episode);
			continue;
		}

		m_subtitleOverrides[episode] = std::move(overrides);
	}

	logger.information("Successfully loaded caption overrides for %s episodes!",
	                   std::to_string(m_subtitleOverrides.size()));
}

void SubtitleOverride::uninitialize()
{
	m_subtitleOverrides.clear();
}

std::string SubtitleOverride::extractCaptionKey(const std::string& fileName)
{
	const std::string baseName = Path(fileName).getFileName();
	const auto underscorePos = baseName.find_last_of('_');
	const auto dotPos = baseName.find_last_of('.');
	return baseName.substr(0, std::min(underscorePos, dotPos));
}

void SubtitleOverride::parseJsonOverride(const std::string& path, const std::string& fileName,
                                         const std::string& episode,
                                         std::map<std::string, std::vector<std::string>>& overrides)
{
	Logger& logger = Logger::get("Server");

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

	JSONArray::Ptr segmentsArray = jsonObject->getArray("segments");
	if (segmentsArray.isNull())
	{
		logger.warning("No segments found in subtitle override file: %s", path);
		return;
	}

	std::vector<std::string> segments;
	for (size_t i = 0; i < segmentsArray->size(); ++i)
		segments.push_back(segmentsArray->getElement<std::string>(static_cast<unsigned int>(i)));

	std::string captionKey = extractCaptionKey(fileName);
	overrides[captionKey] = std::move(segments);

	if (jsonObject->has("episode_title"))
		m_episodeNames[episode] = jsonObject->getValue<std::string>("episode_title");

	logger.debug("Loaded %s caption overrides from JSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode);
}

void SubtitleOverride::parseBsonOverride(const std::string& path, const std::string& fileName,
                                         const std::string& episode,
                                         std::map<std::string, std::vector<std::string>>& overrides)
{
	Logger& logger = Logger::get("Server");

	std::string captionKey = extractCaptionKey(fileName);
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
	MongoDocument document;
	document.read(reader);
	file.close();

	std::vector<std::string> segments;
	if (document.isType<MongoArray::Ptr>("segments"))
	{
		auto& array = document.get<MongoArray::Ptr>("segments");
		for (size_t i = 0; i < array->size(); ++i)
			segments.push_back(array->get<std::string>(i));
	}

	overrides[captionKey] = std::move(segments);

	if (document.exists("episode_title"))
		m_episodeNames[episode] = document.get<std::string>("episode_title");

	logger.debug("Loaded %s caption overrides from GSON for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode);
}

std::string SubtitleOverride::OverrideSubtitles(const std::string& episodeId, const std::string& trackName,
                                                std::string& dataRaw,
                                                bool appendEpTitle)
{
	if (!m_subtitleOverrides.contains(episodeId))
		return dataRaw;

	auto episodeOverrides = m_subtitleOverrides[episodeId];

	if (!episodeOverrides.contains(trackName))
		return dataRaw;

	Logger& logger = Logger::get("Server");

	auto& segments = episodeOverrides[trackName];

	std::istringstream xmlStream(dataRaw);
	InputSource src(xmlStream);
	DOMParser parser;
	AutoPtr doc = parser.parse(&src);
	AutoPtr root = doc->documentElement();
	AutoPtr divs = root->getChildElement("body")->getElementsByTagName("div");

	AutoPtr firstDiv = dynamic_cast<Element*>(divs->item(0));

	if (_episodeNames && appendEpTitle)
	{
		// Create new <p> element with attributes
		AutoPtr p = doc->createElement("p");
		p->setAttribute("xml:id", "episode_title");
		p->setAttribute("begin", "00:00:00.000");
		p->setAttribute("end", "00:00:01.850");
		p->setAttribute("region", "speaker");

		// Create <span> child with style and text content
		AutoPtr span = doc->createElement("span");
		span->setAttribute("style", "textStyle");

		if (m_episodeNames.contains(episodeId))
		{
			AutoPtr text = doc->createTextNode(m_episodeNames[episodeId]);
			span->appendChild(text);

			// Append <span> to <p>
			p->appendChild(span);

			// Add to first <div>
			firstDiv->appendChild(p);
		}
	}

	for (unsigned long i = 0; i < divs->length(); ++i)
	{
		auto div = dynamic_cast<Element*>(divs->item(i));
		AutoPtr ps = div->getElementsByTagName("p");

		for (unsigned long j = 0; j < ps->length(); ++j)
		{
			auto p = dynamic_cast<Element*>(ps->item(j));
			auto span = dynamic_cast<Element*>(p->getElementsByTagName("span")->item(0));

			std::string text = span->innerText();
			std::string origText = text;

			std::string segmentIdStr = p->getAttribute("xml:id");

			if (segmentIdStr != "episode_title")
			{
				if (int segmentId = std::stoi(segmentIdStr.substr(1)); segmentId >= 0 && segmentId < segments.size())
					text = segments[segmentId];
			}

			if (!_closedCaptioning)
			{
				// Remove closed captioning
				std::regex ccRegex(R"([ -]*\[ .* \])");
				text = std::regex_replace(text, ccRegex, "");
			}

			if (!_musicNotes)
			{
				// Remove music notes
				std::string pattern;
				pattern += "\xE2\x99\xAA"; // UTF-8 encoding of music note (U+266A)
				pattern += "\xE2\x99\xAA";

				std::regex musicNoteRegex(pattern);
				text = std::regex_replace(text, musicNoteRegex, "");
			}

			while (span->firstChild())
				span->removeChild(span->firstChild());

			AutoPtr newText = doc->createTextNode(text);
			span->appendChild(newText);

			logger.trace("Episode: %s (%s), Segment: %s ('%s' -> '%s')", episodeId, trackName, segmentIdStr, origText,
			             text);
		}
	}

	DOMWriter writer;
	writer.setOptions(XMLWriter::WRITE_XML_DECLARATION | XMLWriter::CANONICAL_XML);

	std::ostringstream outputStream;
	writer.writeNode(outputStream, doc);

	std::string resultXml = outputStream.str();

	return resultXml;
}
