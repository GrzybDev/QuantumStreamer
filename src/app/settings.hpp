#pragma once

#include <map>
#include <string>

#include "subtitles/subtitle_overrides.hpp"

namespace Poco::Util
{
	class AbstractConfiguration;
}

namespace qs
{
	/// Everything the hook reads from the game's configuration file.
	struct Settings
	{
		struct Logging
		{
			bool showConsole;
			bool saveToFile;
			std::string file;
			std::map<std::string, int> levels;
		};

		struct Server
		{
			unsigned short port;
			int maxQueued;
			int maxThreads;
			bool offlineMode;
			std::string episodesPath;
			std::string videoListPath;
		};

		Logging logging;
		Server server;
		SubtitleOptions subtitles;
		bool patchVideoList;

		/// Reads the settings, falling back to the documented defaults.
		/// @param config The application's configuration.
		static Settings load(const Poco::Util::AbstractConfiguration& config);
	};
}
