#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Poco/Path.h>

#include "media/mp4.hpp"

namespace qs
{
	/// Episodes stored on disk in the Smooth Streaming server layout, served instead of the CDN.
	class OfflineLibrary
	{
	public:
		/// Indexes the episodes that have their own folder under the episodes path.
		/// @param episodesPath The folder holding one subfolder per episode.
		/// @param episodeIds The episodes to look for.
		OfflineLibrary(const std::string& episodesPath, const std::vector<std::string>& episodeIds);

		/// Reads the stored client manifest of an episode.
		/// @param episodeId The episode to look up.
		[[nodiscard]] std::optional<std::string> clientManifest(std::string_view episodeId) const;

		/// Reads a stored moof + mdat fragment of one of an episode's tracks.
		/// @param episodeId The episode to look up.
		/// @param track The stream name, such as video or enus_captions.
		/// @param bitrate The quality level's bitrate.
		/// @param startTime The fragment's start time.
		[[nodiscard]] std::optional<std::string> fragment(std::string_view episodeId, std::string_view track,
		                                                  std::string_view bitrate, std::uint64_t startTime) const;

	private:
		struct Track
		{
			Poco::Path file;
			mp4::FragmentIndex fragments;
		};

		struct Episode
		{
			Poco::Path clientManifest;
			std::map<std::string, Track, std::less<>> tracks;
		};

		static std::optional<Episode> loadEpisode(const std::string& episodeId, const Poco::Path& folder,
		                                          const Poco::Path& serverManifest);
		static std::optional<Track> loadTrack(const Poco::Path& file);

		std::map<std::string, Episode, std::less<>> episodes_;
	};
}
