#include "pch.hpp"
#include "logging.hpp"

namespace qs
{
	namespace
	{
		void attachConsole()
		{
			AllocConsole();

			FILE* stream = nullptr;
			freopen_s(&stream, "CONOUT$", "w", stdout);
			freopen_s(&stream, "CONOUT$", "w", stderr);
			freopen_s(&stream, "CONIN$", "r", stdin);

			std::cout.clear();
			std::clog.clear();
			std::cerr.clear();
			std::cin.clear();
		}
	}

	void setupLogging(const Settings::Logging& settings)
	{
		if (settings.showConsole)
			attachConsole();

		Poco::AutoPtr splitter = new Poco::SplitterChannel;
		splitter->addChannel(new Poco::ConsoleChannel);

		if (settings.saveToFile)
		{
			Poco::AutoPtr file = new Poco::FileChannel(settings.file);
			file->setProperty("rotateOnOpen", "true");
			file->setProperty("archive", "timestamp");
			file->setProperty("compress", "true");
			splitter->addChannel(file);
		}

		const Poco::AutoPtr formatter = new Poco::PatternFormatter("[%Y-%m-%d %H:%M:%S.%i][%p][%s] %t");
		const Poco::AutoPtr channel = new Poco::FormattingChannel(formatter, splitter);

		for (const auto& [name, level] : settings.levels)
		{
			Poco::Logger& logger = Poco::Logger::get(name);
			logger.setChannel(channel);
			logger.setLevel(level);
		}
	}
}
