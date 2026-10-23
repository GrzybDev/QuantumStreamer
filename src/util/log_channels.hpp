#pragma once

#include <array>

namespace qs
{
	/// Names of the loggers, each with its own level in the configuration.
	namespace channel
	{
		inline constexpr auto Core = "Core";
		inline constexpr auto Network = "Network";
		inline constexpr auto VideoList = "VideoList";
		inline constexpr auto OfflineStreaming = "OfflineStreaming";
		inline constexpr auto SubtitleOverride = "SubtitleOverride";

		inline constexpr std::array All{Core, Network, VideoList, OfflineStreaming, SubtitleOverride};
	}
}
