#include "pch.hpp"
#include "settings.hpp"

#include "logging.hpp"

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

	Settings Settings::load(const Poco::Util::AbstractConfiguration& config)
	{
		Settings settings{
			.logging = {
				.showConsole = config.getBool("Logger.ShowConsole", false),
				.saveToFile = config.getBool("Logger.SaveToLogFile", false),
				.file = config.getString("Logger.LogFile", "QuantumStreamer.log"),
				.levels = {},
			},
			.server = {
				.port = config.getUInt16("Server.Port", 0),
				.maxQueued = config.getInt("Server.MaxQueued", 100),
				.maxThreads = config.getInt("Server.MaxThreads", defaultThreadCount()),
				.offlineMode = config.getBool("Server.OfflineMode", false),
				.episodesPath = config.getString("Server.EpisodesPath", "./videos/episodes"),
				.videoListPath = config.getString("Server.VideoListPath", "./data/videoList_original.rmdj"),
			},
			.subtitles = {
				.closedCaptioning = config.getBool("Subtitles.ClosedCaptioning", false),
				.musicNotes = config.getBool("Subtitles.MusicNotes", true),
			},
			.patchVideoList = config.getBool("VideoList.PatchFile", true),
		};

		for (const char* channel : channel::All)
			settings.logging.levels[channel] = config.getInt(std::format("Logger.LogLevel_{}", channel),
			                                                 Poco::Message::PRIO_INFORMATION);

		return settings;
	}
}
