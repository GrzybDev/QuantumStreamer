#pragma once

#include <map>
#include <string>
#include <vector>

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
			bool showConsole = false;
			bool saveToFile = false;
			std::string file = "QuantumStreamer.log";
			std::map<std::string, int> levels;
			std::vector<std::string> invalidValues;

			/// Reads the logging settings; a value that cannot be read keeps its documented default and is listed in
			/// invalidValues.
			/// @param config The application's configuration.
			static Logging load(const Poco::Util::AbstractConfiguration& config);
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

		Server server;
		SubtitleOptions subtitles;
		bool patchVideoList;

		/// Reads the server, caption and video list settings, falling back to the documented defaults.
		/// @param config The application's configuration.
		static Settings load(const Poco::Util::AbstractConfiguration& config);
	};
}
