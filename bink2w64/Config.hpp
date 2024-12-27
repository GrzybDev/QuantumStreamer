#pragma once

using boost::property_tree::ptree;

class Config
{
public:
	Config();

	static Config& GetInstance()
	{
		static Config* instance;

		if (instance == nullptr)
			instance = new Config();

		return *instance;
	}

	struct ServerConfig
	{
		USHORT serverPort;

		bool debugConsole;
		bool debugLogFile;
		std::string debugLogPath;
		int debugConsoleLogLevel;
		int debugLogFileLevel;

		bool subtitlesClosedCaptioning;
		bool subtitlesMusicNotes;
	};

	ServerConfig* cfg = new ServerConfig();
};
