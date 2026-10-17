#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace qs
{
	/// Encrypts or decrypts the contents of an .rmdj file; the cipher is symmetric.
	/// @param data The contents to transform in place.
	void applyRmdjCipher(std::string& data);

	/// The game's encrypted list of episodes, each with the URL of its Smooth Streaming manifest.
	class VideoList
	{
	public:
		/// Reads an encrypted video list.
		/// @param path The list to read.
		/// @return The list, empty when the file cannot be read.
		static VideoList load(const std::string& path);

		/// Points the game's video list at the local server, first keeping the store's copy as the original.
		/// @param originalPath Where the store's copy is kept.
		/// @param port The port the local server listens on.
		/// @return The original list, which still holds the CDN URLs.
		static VideoList patchGameFile(const std::string& originalPath, unsigned short port);

		[[nodiscard]] std::vector<std::string> episodeIds() const;

		/// Looks up the CDN URL of an episode's manifest.
		/// @param episodeId The episode to look up.
		[[nodiscard]] std::optional<std::string> manifestUrl(std::string_view episodeId) const;

		/// Builds the CDN URL of one fragment of an episode.
		/// @param episodeId The episode the fragment belongs to.
		/// @param bitrate The quality level's bitrate.
		/// @param track The stream name, such as video or enus_captions.
		/// @param startTime The fragment's start time.
		[[nodiscard]] std::optional<std::string> fragmentUrl(std::string_view episodeId, std::string_view bitrate,
		                                                     std::string_view track, std::string_view startTime) const;

	private:
		std::map<std::string, std::string, std::less<>> manifestUrls_;
	};
}
