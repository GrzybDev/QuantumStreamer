#include "pch.hpp"
#include "content_sources.hpp"

#include "catalog/video_list.hpp"
#include "media/mp4.hpp"
#include "offline/offline_library.hpp"
#include "subtitles/subtitle_overrides.hpp"
#include "util/log_channels.hpp"

namespace qs
{
	namespace
	{
		std::optional<std::uint64_t> parseStartTime(const std::string_view text)
		{
			std::uint64_t value = 0;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
			if (error != std::errc{} || end != text.data() + text.size())
				return std::nullopt;

			return value;
		}
	}

	ManifestSource::ManifestSource(const Services& services, std::string episodeId) :
		services_(services),
		episodeId_(std::move(episodeId))
	{
	}

	std::string ManifestSource::describe() const
	{
		return std::format("client manifest of episode {}", episodeId_);
	}

	std::optional<std::string> ManifestSource::remoteUrl() const
	{
		return services_.videoList.manifestUrl(episodeId_);
	}

	std::optional<std::string> ManifestSource::loadLocal() const
	{
		return services_.offlineLibrary.clientManifest(episodeId_);
	}

	FragmentSource::FragmentSource(const Services& services, FragmentRequest request) :
		services_(services),
		request_(std::move(request)),
		startTime_(parseStartTime(request_.startTime))
	{
	}

	std::string FragmentSource::describe() const
	{
		return std::format("fragment [episode_id: {}, bitrate: {}, type: {}, start_time: {}]", request_.episodeId,
		                   request_.bitrate, request_.track, request_.startTime);
	}

	std::optional<std::string> FragmentSource::remoteUrl() const
	{
		return services_.videoList.fragmentUrl(request_.episodeId, request_.bitrate, request_.track,
		                                       request_.startTime);
	}

	std::optional<std::string> FragmentSource::loadLocal() const
	{
		if (!startTime_)
			return std::nullopt;

		return services_.offlineLibrary.fragment(request_.episodeId, request_.track, request_.bitrate, *startTime_);
	}

	std::string FragmentSource::transform(std::string content) const
	{
		if (!startTime_ || !services_.subtitles.overrides(request_.episodeId, request_.track))
			return content;

		try
		{
			const auto [moof, ttml] = mp4::splitFragment(content);
			return mp4::joinFragment(
				moof, services_.subtitles.apply(request_.episodeId, request_.track, *startTime_, ttml));
		}
		catch (const Poco::Exception& error)
		{
			Poco::Logger::get(channel::SubtitleOverride).warning(
				std::format("Cannot override the captions of the {} ({})", describe(), error.displayText()));
		}
		catch (const mp4::FormatError& error)
		{
			Poco::Logger::get(channel::SubtitleOverride).warning(
				std::format("Cannot override the captions of the {} ({})", describe(), error.what()));
		}

		return content;
	}
}
