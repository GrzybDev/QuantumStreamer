#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace qs
{
	class OfflineLibrary;
	class SubtitleOverrides;
	class VideoList;

	/// What the request handlers serve episodes from.
	struct Services
	{
		const VideoList& videoList;
		const OfflineLibrary& offlineLibrary;
		const SubtitleOverrides& subtitles;
		bool offlineMode;
	};

	/// Something the game requests, served from disk when stored there and from the CDN otherwise.
	class ContentSource
	{
	public:
		virtual ~ContentSource() = default;

		/// Describes what is requested, for the logs.
		[[nodiscard]] virtual std::string describe() const = 0;

		/// Looks up the CDN URL, which is missing when the episode is unknown.
		[[nodiscard]] virtual std::optional<std::string> remoteUrl() const = 0;

		/// Reads the stored copy, which is missing when it is not on disk.
		[[nodiscard]] virtual std::optional<std::string> loadLocal() const = 0;

		/// Adjusts the content before it reaches the game.
		/// @param content The stored or downloaded content.
		[[nodiscard]] virtual std::string transform(std::string content) const
		{
			return content;
		}
	};

	/// The client manifest of an episode.
	class ManifestSource final : public ContentSource
	{
	public:
		/// Creates the source of one episode's client manifest.
		/// @param services What the manifest is served from.
		/// @param episodeId The episode whose manifest is requested.
		ManifestSource(const Services& services, std::string episodeId);

		[[nodiscard]] std::string describe() const override;
		[[nodiscard]] std::optional<std::string> remoteUrl() const override;
		[[nodiscard]] std::optional<std::string> loadLocal() const override;

	private:
		Services services_;
		std::string episodeId_;
	};

	/// The parts of a fragment URL, as the game sends them.
	struct FragmentRequest
	{
		std::string episodeId;
		std::string bitrate;
		std::string track;
		std::string startTime;
	};

	/// A fragment of one of an episode's streams, with caption overrides applied.
	class FragmentSource final : public ContentSource
	{
	public:
		/// Creates the source of one requested fragment.
		/// @param services What the fragment is served from.
		/// @param request The requested fragment.
		FragmentSource(const Services& services, FragmentRequest request);

		[[nodiscard]] std::string describe() const override;
		[[nodiscard]] std::optional<std::string> remoteUrl() const override;
		[[nodiscard]] std::optional<std::string> loadLocal() const override;
		[[nodiscard]] std::string transform(std::string content) const override;

	private:
		Services services_;
		FragmentRequest request_;
		std::optional<std::uint64_t> startTime_;
	};
}
