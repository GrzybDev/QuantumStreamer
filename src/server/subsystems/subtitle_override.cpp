#include "pch.hpp"
#include "subtitle_override.hpp"

#include "video_list.hpp"
#include <Poco/String.h>
#include <algorithm>
#include <cmath>

using Poco::AutoPtr;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Logger;
using Poco::Path;
using Poco::Util::Application;
using Poco::XML::DOMWriter;
using Poco::XML::Element;
using Poco::XML::InputSource;
using Poco::XML::DOMParser;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::XMLWriter;

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
}

void SubtitleOverride::load()
{
	const Application& app = Application::instance();

	Logger& logger = Logger::get(name());
	logger.debug("Loading subtitle overrides...");

	closed_captioning_ = app.config().getBool("Subtitles.ClosedCaptioning", false);
	music_notes_ = app.config().getBool("Subtitles.MusicNotes", true);

	logger.information("Closed captioning is %s", std::string(closed_captioning_ ? "enabled" : "disabled"));
	logger.information("Music notes are %s", std::string(music_notes_ ? "enabled" : "disabled"));

	const std::string episodesPath = app.config().getString("Server.EpisodesPath", "./videos/episodes");
	VideoList& videoList = app.getSubsystem<VideoList>();

	// Check if the episodes path exists
	for (const auto episodes = videoList.getEpisodeList(); const auto& episodeId : episodes)
	{
		File episodeDir(episodesPath + "/" + episodeId);
		if (!(episodeDir.exists() && episodeDir.isDirectory())) continue;

		std::map<std::string, std::vector<SrtSegment>> overrides;

		for (DirectoryIterator it(episodeDir), end; it != end; ++it)
		{
			const auto& filePath = it.path();
			const std::string& fileName = filePath.getFileName();
			std::string extension = Path(fileName).getExtension();

			if (fileName.find("_captions") == std::string::npos) continue;

			if (extension == "srt")
				parseSrtOverride(filePath.toString(), fileName, episodeId, overrides);
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
	return Path(file_name).getBaseName();
}

double SubtitleOverride::parseSrtTime(const std::string& time_str)
{
	// Format: HH:MM:SS,MMM
	int h = 0, m = 0;
	double s = 0.0;

	auto firstColon = time_str.find(':');
	auto secondColon = time_str.find(':', firstColon + 1);

	if (firstColon != std::string::npos && secondColon != std::string::npos)
	{
		h = std::stoi(time_str.substr(0, firstColon));
		m = std::stoi(time_str.substr(firstColon + 1, secondColon - firstColon - 1));
		std::string secStr = time_str.substr(secondColon + 1);
		std::replace(secStr.begin(), secStr.end(), ',', '.');
		s = std::stod(secStr);
	}

	return h * 3600.0 + m * 60.0 + s;
}

double SubtitleOverride::parseTtmlTime(const std::string& time_str)
{
	// Format: HH:MM:SS.MMM
	int h = 0, m = 0;
	double s = 0.0;

	auto firstColon = time_str.find(':');
	auto secondColon = time_str.find(':', firstColon + 1);

	if (firstColon != std::string::npos && secondColon != std::string::npos)
	{
		h = std::stoi(time_str.substr(0, firstColon));
		m = std::stoi(time_str.substr(firstColon + 1, secondColon - firstColon - 1));
		s = std::stod(time_str.substr(secondColon + 1));
	}

	return h * 3600.0 + m * 60.0 + s;
}

std::string SubtitleOverride::formatTtmlTime(double time_sec)
{
	int h = static_cast<int>(time_sec / 3600);
	int m = static_cast<int>(std::fmod(time_sec, 3600) / 60);
	double s = std::fmod(time_sec, 60);

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%02d:%02d:%06.3f", h, m, s);
	return std::string(buffer);
}

void SubtitleOverride::parseSrtOverride(const std::string& path, const std::string& file_name,
                                        const std::string& episode_id,
                                        std::map<std::string, std::vector<SrtSegment>>& overrides)
{
	Logger& logger = Logger::get(name());

	std::ifstream file(path);
	if (!file)
	{
		logger.error("Failed to open subtitle override file: %s", path);
		return;
	}

	std::vector<SrtSegment> segments;
	std::string line;

	while (std::getline(file, line))
	{
		Poco::trimInPlace(line);
		if (line.empty()) continue;

		// The next line is the timestamps
		std::string timeLine;
		if (!std::getline(file, timeLine)) break;
		Poco::trimInPlace(timeLine);

		auto arrowPos = timeLine.find(" --> ");
		if (arrowPos == std::string::npos) continue;

		std::string beginTimeStr = timeLine.substr(0, arrowPos);
		std::string endTimeStr = timeLine.substr(arrowPos + 5);

		double beginTime = parseSrtTime(beginTimeStr);
		double endTime = parseSrtTime(endTimeStr);

		// The next lines are the text until an empty line
		std::string text;
		while (std::getline(file, line))
		{
			Poco::trimInPlace(line);
			if (line.empty()) break;
			if (!text.empty()) text += "\n";
			text += line;
		}

		segments.push_back({beginTime, endTime, text});
	}

	std::string captionKey = extractCaptionKey(file_name);
	overrides[captionKey] = std::move(segments);

	logger.debug("Loaded %s caption overrides for track %s in episode %s",
	             std::to_string(overrides[captionKey].size()), captionKey, episode_id);
}

std::string SubtitleOverride::overrideSubtitles(const std::string& episode_id, const std::string& track_name,
                                                std::string& data_raw, const std::string& start_time)
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
	NodeList* divList = doc->getElementsByTagName("div");
	if (divList->length() == 0) return data_raw;
	auto firstDiv = dynamic_cast<Element*>(divList->item(0));

	NodeList* pList = doc->getElementsByTagName("p");

	double frag_time_sec = 0.0;
	if (!start_time.empty())
		frag_time_sec = std::stoull(start_time) / 10000000.0;

	// Calculate maximum relative end time to estimate fragment duration
	double max_rel_end = 0.0;
	std::vector<Node*> nodesToRemove;
	for (unsigned long i = 0; i < pList->length(); ++i)
	{
		auto pElem = dynamic_cast<Element*>(pList->item(i));
		double rel_end = parseTtmlTime(pElem->getAttribute("end"));
		if (rel_end > max_rel_end)
			max_rel_end = rel_end;
		nodesToRemove.push_back(pElem);
	}

	// Remove existing <p> elements
	for (auto node : nodesToRemove)
	{
		node->parentNode()->removeChild(node);
		node->release();
	}

	// Default duration is at least 2.5 seconds to ensure sufficient overlap window
	double fragment_duration = std::max(max_rel_end, 2.5);

	int pId = 1;
	for (const auto& seg : segments)
	{
		// Check if the SRT segment overlaps with the current fragment
		if (seg.end_time_sec > frag_time_sec && seg.begin_time_sec < frag_time_sec + fragment_duration)
		{
			double rel_begin = std::max(0.0, seg.begin_time_sec - frag_time_sec);
			double rel_end = seg.end_time_sec - frag_time_sec;

			AutoPtr p = doc->createElement("p");
			p->setAttribute("xml:id", "p" + std::to_string(pId++));
			p->setAttribute("begin", formatTtmlTime(rel_begin));
			p->setAttribute("end", formatTtmlTime(rel_end));
			p->setAttribute("region", "speaker");

			AutoPtr span = doc->createElement("span");
			span->setAttribute("style", "textStyle");

			std::string newText = seg.text;

			if (!closed_captioning_)
			{
				std::regex ccRegex(R"([ -]*\[ .* \])");
				newText = std::regex_replace(newText, ccRegex, "");
			}

			if (!music_notes_)
			{
				std::string pattern;
				pattern += "\xE2\x99\xAA";
				pattern += "\xE2\x99\xAA";
				std::regex musicNoteRegex(pattern);
				newText = std::regex_replace(newText, musicNoteRegex, "");
			}

			AutoPtr textNode = doc->createTextNode(newText);
			span->appendChild(textNode);
			p->appendChild(span);

			firstDiv->appendChild(p);

			logger.trace("Episode: %s (%s), Subtitle Segment: p%d ('%s')", episode_id, track_name, pId - 1, newText);
		}
	}

	DOMWriter writer;
	writer.setOptions(XMLWriter::WRITE_XML_DECLARATION | XMLWriter::CANONICAL_XML);

	std::ostringstream outputStream;
	writer.writeNode(outputStream, doc);

	return outputStream.str();
}
