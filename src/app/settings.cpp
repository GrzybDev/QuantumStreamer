#include "pch.hpp"
#include "settings.hpp"

#include "util/log_channels.hpp"

namespace qs
{
	namespace
	{
		int defaultThreadCount()
		{
			const unsigned int count = std::thread::hardware_concurrency();
			return count > 0 ? static_cast<int>(count) : 2;
		}
	}

	Settings::Logging Settings::Logging::load(const Poco::Util::AbstractConfiguration& config)
	{
		Logging logging;

		// Logging cannot report its own invalid values, so each keeps its default and is reported once logging runs.
		const auto read = [&](const std::string& key, auto& value)
		{
			try
			{
				if constexpr (std::is_same_v<std::remove_reference_t<decltype(value)>, bool>)
					value = config.getBool(key, value);
				else
					value = config.getInt(key, value);
			}
			catch (const Poco::Exception& error)
			{
				logging.invalidValues.push_back(std::format("{} ({})", key, error.displayText()));
			}
		};

		read("Logger.ShowConsole", logging.showConsole);
		read("Logger.SaveToLogFile", logging.saveToFile);
		logging.file = config.getString("Logger.LogFile", logging.file);
		for (const char* channel : channel::All)
			read(std::format("Logger.LogLevel_{}", channel), logging.levels[channel] = Poco::Message::PRIO_INFORMATION);

		return logging;
	}

	Settings Settings::load(const Poco::Util::AbstractConfiguration& config)
	{
		constexpr SubtitleOptions defaults;
		return {
			.server = {
				.port = config.getUInt16("Server.Port", 0),
				.maxQueued = config.getInt("Server.MaxQueued", 100),
				.maxThreads = config.getInt("Server.MaxThreads", defaultThreadCount()),
				.offlineMode = config.getBool("Server.OfflineMode", false),
				.episodesPath = config.getString("Server.EpisodesPath", "./videos/episodes"),
				.videoListPath = config.getString("Server.VideoListPath", "./data/videoList_original.rmdj"),
			},
			.subtitles = {
				.closedCaptioning = config.getBool("Subtitles.ClosedCaptioning", defaults.closedCaptioning),
				.musicNotes = config.getBool("Subtitles.MusicNotes", defaults.musicNotes),
			},
			.patchVideoList = config.getBool("VideoList.PatchFile", true),
		};
	}
}
