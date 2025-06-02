#include "pch.hpp"
#include "SubtitleOverride.hpp"

#include "VideoList.hpp"

using Poco::Logger;
using Poco::Util::Application;

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

	std::string episodesPath = app.config().getString("Server.EpisodesPath", "./videos/episodes");
	VideoList& videoList = app.getSubsystem<VideoList>();

	auto episodes = videoList.getEpisodeList();

	// Check if the episodes path exists
	for (const auto& episode : episodes)
	{
		auto episodePath = Poco::File(episodesPath + "/" + episode);

		if (!(episodePath.exists() && episodePath.isDirectory()))
			continue;

		std::map<std::string, std::vector<std::string>> overrides;

		// Find *_captions_override.json or *_captions_override.bson files
		Poco::DirectoryIterator end;

		for (Poco::DirectoryIterator it(episodePath); it != end; ++it)
		{
			std::string fileName = it.name();
			if ((Poco::Path(fileName).getExtension() == "json") &&
				fileName.find("_captions_override") != std::string::npos &&
				fileName.size() >= strlen("_captions_override.json"))
			{
				std::ifstream overrideFile(it.path().toString());

				if (!overrideFile)
				{
					logger.error("Failed to open subtitle override file: %s", it.path().toString());
					continue;
				}

				Poco::JSON::Parser parser;
				Poco::Dynamic::Var result = parser.parse(overrideFile);
				auto jsonObject = result.extract<Poco::JSON::Object::Ptr>();

				overrideFile.close();

				Poco::JSON::Array::Ptr segmentsArray = jsonObject->getArray("segments");
				if (segmentsArray.isNull())
				{
					logger.warning("No segments found in subtitle override file: %s", it.path().toString());
					continue;
				}

				std::vector<std::string> segments;

				for (size_t i = 0; i < segmentsArray->size(); ++i)
				{
					auto segment = segmentsArray->getElement<std::string>(i);
					segments.push_back(segment);
				}

				// Key is the fileName without extension and _override
				std::string captionKey = Poco::Path(fileName).getFileName();
				captionKey = captionKey.substr(0, captionKey.find_last_of('_'));
				captionKey = captionKey.substr(0, captionKey.find_last_of('.'));
				overrides[captionKey] = segments;

				if (jsonObject->has("episode_title"))
				{
					auto episodeTitle = jsonObject->getValue<std::string>("episode_title");
					m_episodeNames[episode] = episodeTitle;
				}

				logger.debug(
					"Successfully loaded caption overrides for episode %s! (Found overrides for %s captions for track %s)",
					episode, std::to_string(overrides[captionKey].size()), captionKey);
			}
		}

		// Now the same for BSON files
		for (Poco::DirectoryIterator it(episodePath); it != end; ++it)
		{
			std::string fileName = it.name();
			if ((Poco::Path(fileName).getExtension() == "bson") &&
				fileName.find("_captions_override") != std::string::npos &&
				fileName.size() >= strlen("_captions_override.bson"))
			{
				// Skip if caption overrides already loaded from JSON
				std::string captionKey = Poco::Path(fileName).getFileName();
				captionKey = captionKey.substr(0, captionKey.find_last_of('_'));
				captionKey = captionKey.substr(0, captionKey.find_last_of('.'));

				// If we already have this caption key from JSON, skip BSON loading
				if (overrides.contains(captionKey))
				{
					logger.debug("Skipping BSON caption overrides for %s, already loaded from JSON", captionKey);
					continue;
				}

				std::ifstream overrideFile(it.path().toString(), std::ios::binary);
				if (!overrideFile)
				{
					logger.error("Failed to open BSON subtitle override file: %s", it.path().toString());
					continue;
				}

				Poco::BinaryReader reader(overrideFile);

				Poco::MongoDB::Document document;
				document.read(reader);

				overrideFile.close();

				std::vector<std::string> segments;

				// Read segments from BSON document
				if (document.isType<Poco::MongoDB::Array::Ptr>("segments"))
				{
					auto segmentArray = document.get<Poco::MongoDB::Array::Ptr>("segments");

					for (size_t i = 0; i < segmentArray->size(); ++i)
					{
						auto segment = segmentArray->get<std::string>(i);
						segments.push_back(segment);
					}
				}

				overrides[captionKey] = segments;

				if (document.exists("episode_title"))
				{
					auto episodeTitle = document.get<std::string>("episode_title");
					m_episodeNames[episode] = episodeTitle;
				}

				logger.debug(
					"Successfully loaded caption overrides for episode %s! (Found overrides for %s captions for track %s)",
					episode, std::to_string(overrides[captionKey].size()), captionKey);
			}
		}

		m_subtitleOverrides[episode] = overrides;
	}

	logger.information("Successfully loaded caption overrides for %s episodes!",
	                   std::to_string(m_subtitleOverrides.size()));
}

void SubtitleOverride::uninitialize()
{
	m_subtitleOverrides.clear();
}

std::string SubtitleOverride::OverrideSubtitles(std::string episodeId, std::string trackName, std::string& dataRaw,
                                                bool appendEpTitle)
{
	if (!m_subtitleOverrides.contains(episodeId))
		return dataRaw;

	auto episodeOverrides = m_subtitleOverrides[episodeId];

	if (!episodeOverrides.contains(trackName))
		return dataRaw;

	Logger& logger = Logger::get("Server");

	auto segments = episodeOverrides[trackName];

	std::istringstream xmlStream(dataRaw);
	Poco::XML::InputSource src(xmlStream);
	Poco::XML::DOMParser parser;
	Poco::AutoPtr doc = parser.parse(&src);
	Poco::AutoPtr root = doc->documentElement();
	Poco::AutoPtr divs = root->getChildElement("body")->getElementsByTagName("div");

	Poco::AutoPtr firstDiv = static_cast<Poco::XML::Element*>(divs->item(0));

	if (_episodeNames && appendEpTitle)
	{
		// Create new <p> element with attributes
		Poco::AutoPtr p = doc->createElement("p");
		p->setAttribute("xml:id", "episode_title");
		p->setAttribute("begin", "00:00:00.000");
		p->setAttribute("end", "00:00:01.850");
		p->setAttribute("region", "speaker");

		// Create <span> child with style and text content
		Poco::AutoPtr span = doc->createElement("span");
		span->setAttribute("style", "textStyle");

		if (m_episodeNames.contains(episodeId))
		{
			Poco::AutoPtr text = doc->createTextNode(m_episodeNames[episodeId]);
			span->appendChild(text);

			// Append <span> to <p>
			p->appendChild(span);

			// Add to first <div>
			firstDiv->appendChild(p);
		}
	}

	for (ULONG i = 0; i < divs->length(); ++i)
	{
		auto div = static_cast<Poco::XML::Element*>(divs->item(i));
		Poco::AutoPtr ps = div->getElementsByTagName("p");

		for (ULONG j = 0; j < ps->length(); ++j)
		{
			auto p = static_cast<Poco::XML::Element*>(ps->item(j));

			auto span = static_cast<Poco::XML::Element*>(p->getElementsByTagName("span")->item(0));
			std::string text = span->innerText();
			std::string origText = text;

			std::string segmentIdStr = p->getAttribute("xml:id");

			if (segmentIdStr != "episode_title")
			{
				int segmentId = std::stoi(segmentIdStr.substr(1));


				if (segmentId >= 0 && segmentId < segments.size())
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
			{
				span->removeChild(span->firstChild());
			}

			Poco::AutoPtr newText = doc->createTextNode(text);
			span->appendChild(newText);

			logger.trace("Episode: %s (%s), Segment: %s ('%s' -> '%s')", episodeId, trackName, segmentIdStr, origText,
			             text);
		}
	}

	Poco::XML::DOMWriter writer;
	writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION | Poco::XML::XMLWriter::CANONICAL_XML);

	std::ostringstream outputStream;
	writer.writeNode(outputStream, doc);

	std::string resultXml = outputStream.str();

	return resultXml;
}
