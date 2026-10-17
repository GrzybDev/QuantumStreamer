#include "pch.hpp"
#include "subtitle_overrides.hpp"

#include "app/logging.hpp"
#include "util/files.hpp"

namespace qs
{
	namespace
	{
		constexpr double kTicksPerSecond = 10'000'000.0;
		// A fragment without timed captions still has to show the cues that start shortly after it.
		constexpr double kMinimumFragmentSeconds = 2.5;
		constexpr auto kDoubleMusicNote = "\xE2\x99\xAA\xE2\x99\xAA";
		constexpr std::string_view kCueArrow = " --> ";

		Poco::Logger& logger()
		{
			return Poco::Logger::get(channel::SubtitleOverride);
		}

		template <typename T>
		bool parseNumber(const std::string_view text, T& value)
		{
			return std::from_chars(text.data(), text.data() + text.size(), value).ec == std::errc{};
		}

		// Reads SubRip (HH:MM:SS,mmm) and TTML (HH:MM:SS.mmm) clock times; anything else counts as zero.
		double parseClockTime(const std::string_view text)
		{
			const std::size_t firstColon = text.find(':');
			const std::size_t secondColon = text.find(':', firstColon + 1);
			if (firstColon == std::string_view::npos || secondColon == std::string_view::npos)
				return 0.0;

			std::string secondsText(text.substr(secondColon + 1));
			std::ranges::replace(secondsText, ',', '.');

			int hours = 0;
			int minutes = 0;
			double seconds = 0.0;
			if (!parseNumber(text.substr(0, firstColon), hours)
				|| !parseNumber(text.substr(firstColon + 1, secondColon - firstColon - 1), minutes)
				|| !parseNumber(std::string_view(secondsText), seconds))
				return 0.0;

			return hours * 3600.0 + minutes * 60.0 + seconds;
		}

		std::string formatClockTime(const double seconds)
		{
			return std::format("{:02}:{:02}:{:06.3f}", static_cast<int>(seconds / 3600),
			                   static_cast<int>(std::fmod(seconds, 3600) / 60), std::fmod(seconds, 60));
		}
	}

	SubtitleOverrides::SubtitleOverrides(const std::string& episodesPath, const std::vector<std::string>& episodeIds,
	                                     const SubtitleOptions options) : options_(options)
	{
		logger().information(std::format("Closed captioning is {}", options.closedCaptioning ? "enabled" : "disabled"));
		logger().information(std::format("Music notes are {}", options.musicNotes ? "enabled" : "disabled"));

		for (const std::string& episodeId : episodeIds)
		{
			const std::optional<Poco::Path> folder = findEpisodeFolder(episodesPath, episodeId);
			if (!folder)
				continue;

			std::map<std::string, Cues, std::less<>> tracks;
			for (Poco::DirectoryIterator file(*folder), end; file != end; ++file)
			{
				if (file.name().find("_captions") == std::string::npos || file.path().getExtension() != "srt")
					continue;

				std::ifstream stream(file.path().toString());
				if (!stream)
				{
					logger().error(std::format("Failed to open subtitle override file: {}", file.path().toString()));
					continue;
				}

				const std::string track = file.path().getBaseName();
				const Cues& cues = tracks[track] = parseSrt(stream);
				logger().debug(std::format("Loaded {} caption overrides for track {} in episode {}", cues.size(), track,
				                           episodeId));
			}

			if (tracks.empty())
				logger().debug(std::format("No subtitle overrides found for episode {}", episodeId));
			else
				episodes_[episodeId] = std::move(tracks);
		}

		logger().information(std::format("Successfully loaded caption overrides for {} episodes!", episodes_.size()));
	}

	SubtitleOverrides::Cues SubtitleOverrides::parseSrt(std::istream& stream)
	{
		Cues cues;
		std::string line;

		while (std::getline(stream, line))
		{
			if (Poco::trimInPlace(line).empty())
				continue;

			// The non-empty line is the cue number, the timing follows it.
			std::string timing;
			if (!std::getline(stream, timing))
				break;

			const std::string_view times = Poco::trimInPlace(timing);
			const std::size_t arrow = times.find(kCueArrow);
			if (arrow == std::string_view::npos)
				continue;

			Cue cue{parseClockTime(times.substr(0, arrow)), parseClockTime(times.substr(arrow + kCueArrow.size())), {}};
			while (std::getline(stream, line) && !Poco::trimInPlace(line).empty())
			{
				if (!cue.text.empty())
					cue.text += '\n';
				cue.text += line;
			}

			cues.push_back(std::move(cue));
		}

		return cues;
	}

	bool SubtitleOverrides::overrides(const std::string_view episodeId, const std::string_view track) const
	{
		return find(episodeId, track) != nullptr;
	}

	std::string SubtitleOverrides::apply(const std::string_view episodeId, const std::string_view track,
	                                     const std::uint64_t startTime, const std::string_view ttml) const
	{
		const Cues* cues = find(episodeId, track);
		if (!cues)
			return std::string(ttml);

		Poco::XML::DOMParser parser;
		const Poco::AutoPtr<Poco::XML::Document> document = parser.parseMemory(ttml.data(), ttml.size());

		const Poco::AutoPtr<Poco::XML::NodeList> divs = document->getElementsByTagName("div");
		if (divs->length() == 0)
			return std::string(ttml);

		auto* body = static_cast<Poco::XML::Element*>(divs->item(0));

		// The node list is live, so the paragraphs are collected before any is removed.
		const Poco::AutoPtr<Poco::XML::NodeList> paragraphs = document->getElementsByTagName("p");
		std::vector<Poco::XML::Node*> originals;
		double fragmentSeconds = kMinimumFragmentSeconds;
		for (unsigned long i = 0; i < paragraphs->length(); ++i)
		{
			auto* paragraph = static_cast<Poco::XML::Element*>(paragraphs->item(i));
			fragmentSeconds = std::max(fragmentSeconds, parseClockTime(paragraph->getAttribute("end")));
			originals.push_back(paragraph);
		}

		// removeChild hands the node to the document's autorelease pool, so releasing it here would free it twice.
		for (Poco::XML::Node* paragraph : originals)
			paragraph->parentNode()->removeChild(paragraph);

		const double fragmentStart = static_cast<double>(startTime) / kTicksPerSecond;
		int paragraphId = 1;
		for (const Cue& cue : *cues)
		{
			if (cue.end <= fragmentStart || cue.begin >= fragmentStart + fragmentSeconds)
				continue;

			const std::string id = std::format("p{}", paragraphId++);
			const std::string text = filter(cue.text);

			// Without namespaces the writer binds the reserved xml prefix to TTML, which strict parsers reject.
			Poco::AutoPtr<Poco::XML::Element> paragraph = document->createElementNS(body->namespaceURI(), "p");
			paragraph->setAttributeNS(Poco::XML::NamespaceSupport::XML_NAMESPACE, "xml:id", id);
			paragraph->setAttribute("begin", formatClockTime(std::max(0.0, cue.begin - fragmentStart)));
			paragraph->setAttribute("end", formatClockTime(cue.end - fragmentStart));
			paragraph->setAttribute("region", "speaker");

			Poco::AutoPtr<Poco::XML::Element> span = document->createElementNS(body->namespaceURI(), "span");
			span->setAttribute("style", "textStyle");

			Poco::AutoPtr<Poco::XML::Text> textNode = document->createTextNode(text);
			span->appendChild(textNode);
			paragraph->appendChild(span);
			body->appendChild(paragraph);

			if (logger().trace())
				logger().trace(std::format("Episode: {} ({}), Subtitle Segment: {} ('{}')", episodeId, track, id, text));
		}

		Poco::XML::DOMWriter writer;
		writer.setOptions(Poco::XML::XMLWriter::WRITE_XML_DECLARATION | Poco::XML::XMLWriter::CANONICAL_XML);

		std::ostringstream output;
		writer.writeNode(output, document);
		return output.str();
	}

	const SubtitleOverrides::Cues* SubtitleOverrides::find(const std::string_view episodeId,
	                                                       const std::string_view track) const
	{
		const auto episode = episodes_.find(episodeId);
		if (episode == episodes_.end())
			return nullptr;

		const auto cues = episode->second.find(track);
		return cues == episode->second.end() ? nullptr : &cues->second;
	}

	std::string SubtitleOverrides::filter(std::string text) const
	{
		if (!options_.closedCaptioning)
		{
			static const std::regex annotation(R"([ -]*\[ .*? \])");
			text = std::regex_replace(text, annotation, "");
		}

		if (!options_.musicNotes)
			Poco::replaceInPlace(text, kDoubleMusicNote, "");

		return text;
	}
}
