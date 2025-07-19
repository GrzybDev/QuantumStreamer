#include "pch.hpp"
#include "main.hpp"

#include "handler_factory.hpp"
#include "subsystems/offline_streaming.hpp"
#include "subsystems/subtitle_override.hpp"
#include "subsystems/video_list.hpp"

using Poco::AutoPtr;
using Poco::ConsoleChannel;
using Poco::FileChannel;
using Poco::FormattingChannel;
using Poco::Logger;
using Poco::Message;
using Poco::PatternFormatter;
using Poco::SplitterChannel;
using Poco::ThreadPool;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::ServerSocket;
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

	addSubsystem(new VideoList);
	addSubsystem(new OfflineStreaming);
	addSubsystem(new SubtitleOverride);

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

	// Custom format for log messages
	const AutoPtr pFormatter = new PatternFormatter("[%Y-%m-%d %H:%M:%S.%i][%p][%s] %t");

	// Create a FormattingChannel that wraps ConsoleChannel
	const AutoPtr pFormattingChannel = new FormattingChannel(pFormatter, pSplitterChannel);

	const int logLevelCore = config().getInt("Logger.LogLevel_Core", Message::PRIO_INFORMATION);
	const int logLevelNetwork = config().getInt("Logger.LogLevel_Network", Message::PRIO_INFORMATION);
	const int logLevelVideoList = config().getInt("Logger.LogLevel_VideoList", Message::PRIO_INFORMATION);
	const int logLevelOfflineStreaming = config().getInt("Logger.LogLevel_OfflineStreaming", Message::PRIO_INFORMATION);
	const int logLevelSubtitleOverride = config().getInt("Logger.LogLevel_SubtitleOverride", Message::PRIO_INFORMATION);

	Logger::create("Core", pFormattingChannel, logLevelCore);
	Logger::create("Network", pFormattingChannel, logLevelNetwork);
	Logger::create("VideoList", pFormattingChannel, logLevelVideoList);
	Logger::create("OfflineStreaming", pFormattingChannel, logLevelOfflineStreaming);
	Logger::create("SubtitleOverride", pFormattingChannel, logLevelSubtitleOverride);
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

int QuantumStreamer::main(const std::vector<std::string>& args)
{
	Logger& logger = Logger::get("Core");
	logger.debug("Initializing Quantum Streamer...");

	unsigned int n = std::thread::hardware_concurrency();

	if (n == 0)
	{
		logger.warning(
			"Could not detect hardware concurrency, will use default value of 2 threads if not configured manually.");
		n = 2; // Default to 2 threads if hardware concurrency cannot be detected
	}

	const int maxQueued = config().getInt("Server.MaxQueued", 100);
	const int maxThreads = config().getInt("Server.MaxThreads", static_cast<int>(n));
	ThreadPool::defaultPool().addCapacity(maxThreads);

	const auto pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	pParams->setMaxQueued(maxQueued);
	pParams->setMaxThreads(maxThreads);
	logger.debug("Max Queued: %d", maxQueued);
	logger.debug("Max Threads: %d", maxThreads);

	// set up the server socket
	const unsigned short port = static_cast<unsigned short>(config().getInt("Server.Port", 0));
	const ServerSocket svs(port);

	if (config().getBool("VideoList.PatchFile", true))
	{
		VideoList& videoList = instance().getSubsystem<VideoList>();
		videoList.patch(svs.address().port());

		OfflineStreaming& offlineStreaming = instance().getSubsystem<OfflineStreaming>();
		offlineStreaming.preload();

		SubtitleOverride& subtitleOverride = instance().getSubsystem<SubtitleOverride>();
		subtitleOverride.load();
	}

	// create the HTTP server instance
	HTTPServer srv(new RequestHandlerFactory(), svs, pParams);

	// start the server
	srv.start();

	logger.information("Started HTTP Server (Listening at port %d)", static_cast<int>(svs.address().port()));

	// wait for termination signal
	waitForTerminationRequest();

	logger.information("Stopping HTTP Server...");

	// stop the server
	srv.stop();
	return EXIT_OK;
}
