#include "pch.hpp"
#include "main.hpp"

using Poco::AutoPtr;
using Poco::ConsoleChannel;
using Poco::FileChannel;
using Poco::SplitterChannel;
using Poco::Util::Application;

void QuantumStreamer::initialize(Application& self)
{
	loadConfiguration();
	setupLogger();

	self.logger().notice("Quantum Streamer %s by Marek Grzyb (@GrzybDev)",
	                     std::format("v{}.{}.{}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH));
	self.logger().notice("Homepage: https://grzyb.dev/app/QuantumStreamer");
	self.logger().notice("Source code: https://github.com/GrzybDev/QuantumStreamer");
	self.logger().notice("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/QuantumStreamer/issues");
	self.logger().notice("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!");

	ServerApplication::initialize(self);
}

void QuantumStreamer::setupLogger() const
{
	const bool showConsole = config().getBool("Logger.ShowConsole", false);
	const bool saveToLogFile = config().getBool("Logger.SaveToLogFile", false);

	if (showConsole)
		setupConsole();

	// Create channels
	const AutoPtr pConsoleChannel = new ConsoleChannel;
	AutoPtr pFileChannel = new FileChannel;

	const std::string logFile = config().getString("Logger.LogFile", "QuantumStreamer.log");
	pFileChannel->setProperty("path", logFile);
	pFileChannel->setProperty("rotateOnOpen", "true");
	pFileChannel->setProperty("archive", "timestamp"); // archive with timestamp
	pFileChannel->setProperty("compress", "true"); // compress log files

	// Create SplitterChannel and add both channels
	AutoPtr pSplitterChannel = new SplitterChannel;
	pSplitterChannel->addChannel(pConsoleChannel);

	if (saveToLogFile)
		pSplitterChannel->addChannel(pFileChannel);
}

void QuantumStreamer::setupConsole()
{
	// Create a console for Debug output
	AllocConsole();

	// Redirect standard error, output to console
	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;

	(void)freopen_s(&fDummy, "CONOUT$", "w", stdout);
	(void)freopen_s(&fDummy, "CONOUT$", "w", stderr);
	(void)freopen_s(&fDummy, "CONIN$", "r", stdin);

	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// Redirect wide standard error, output to console
	// std::wcout, std::wclog, std::wcerr, std::wcin
	const HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE,
	                                  FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
	                                  FILE_ATTRIBUTE_NORMAL, nullptr);
	const HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                                 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);

	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
}
