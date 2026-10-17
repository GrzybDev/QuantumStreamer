#pragma once

#include <array>

#include "settings.hpp"

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

	/// Points every channel at the console and, when enabled, at a rotating log file.
	/// @param settings Where to write and how detailed each channel is.
	void setupLogging(const Settings::Logging& settings);
}
