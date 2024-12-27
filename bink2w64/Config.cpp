// Config.cpp : Defines the configuration settings for the application.
#include "pch.hpp"

Config::Config()
{
	// Define the path to the configuration file
	const auto configPath = "streamer.json";
	ptree pt;

	try
	{
		// Try to read the configuration file
		read_json(configPath, pt);
	}
	catch (boost::property_tree::json_parser_error& e)
	{
		// If there is a parse error, use the default configuration

		if (boost::filesystem::exists(configPath))
		{
			// If the configuration file exists, print the parse error
			const auto message = "Quantum Streamer will use default configs because error occured while parsing config file: " + std::string(e.what());

			BOOST_LOG_TRIVIAL(error) << message;
		}
	}

	cfg->serverPort = pt.get("server.port", DEFAULT_HTTP_PORT);
	cfg->debugConsole = pt.get("debug.showConsole", false);
	cfg->debugLogFile = pt.get("debug.createLog", false);

	const boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
	const auto facet = new boost::posix_time::time_facet("%Y-%m-%d_%H-%M-%S");

	std::ostringstream is;
	is.imbue(std::locale(is.getloc(), facet));
	is << timeLocal;

	cfg->debugLogPath = pt.get("debug.logPath", (boost::format("QuantumStreamer_%1%.log") % is.str()).str());

	cfg->debugConsoleLogLevel = pt.get("debug.logLevelConsole", static_cast<int>(boost::log::trivial::info));
	cfg->debugLogFileLevel = pt.get("debug.logLevelFile", static_cast<int>(boost::log::trivial::debug));

	cfg->subtitlesClosedCaptioning = pt.get("subtitles.closedCaptioning", true);
	cfg->subtitlesMusicNotes = pt.get("subtitles.musicNotes", true);
}