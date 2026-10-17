#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace qs
{
	/// Which annotations overridden captions keep.
	struct SubtitleOptions
	{
		bool closedCaptioning = false;
		bool musicNotes = true;
	};

	/// Caption tracks replaced by SubRip files stored in the episodes' folders.
	class SubtitleOverrides
	{
	public:
		/// Loads the *_captions*.srt files from each episode's folder.
		/// @param episodesPath The folder holding one subfolder per episode.
		/// @param episodeIds The episodes to look for.
		/// @param options Which annotations the captions keep.
		SubtitleOverrides(const std::string& episodesPath, const std::vector<std::string>& episodeIds,
		                  SubtitleOptions options);

		/// Tells whether a caption track of an episode is overridden.
		/// @param episodeId The episode to look up.
		/// @param track The caption stream name, such as enus_captions.
		[[nodiscard]] bool overrides(std::string_view episodeId, std::string_view track) const;

		/// Replaces the captions of one TTML fragment with the override's cues that overlap it.
		/// @param episodeId The episode the fragment belongs to.
		/// @param track The caption stream name, such as enus_captions.
		/// @param startTime The fragment's start time in 100 ns units.
		/// @param ttml The fragment's TTML document.
		/// @return The rewritten document, or the original one when the track is not overridden.
		[[nodiscard]] std::string apply(std::string_view episodeId, std::string_view track, std::uint64_t startTime,
		                                std::string_view ttml) const;

	private:
		struct Cue
		{
			double begin;
			double end;
			std::string text;
		};

		using Cues = std::vector<Cue>;

		static Cues parseSrt(std::istream& stream);

		[[nodiscard]] const Cues* find(std::string_view episodeId, std::string_view track) const;
		[[nodiscard]] std::string filter(std::string text) const;

		std::map<std::string, std::map<std::string, Cues, std::less<>>, std::less<>> episodes_;
		SubtitleOptions options_;
	};
}
