#include "pch.hpp"
#include "StreamingServer.hpp"
#include "RequestHandlerFactory.hpp"

using Poco::AutoPtr;
using Poco::ConsoleChannel;
using Poco::FileChannel;
using Poco::FormattingChannel;
using Poco::Logger;
using Poco::Message;
using Poco::PatternFormatter;
using Poco::SplitterChannel;
using Poco::ThreadPool;

using Poco::Net::ServerSocket;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;


VOID StreamingServer::initialize(Application& self)
{
	loadConfiguration();
	initLoggers();

	self.logger().notice("Quantum Streamer by Marek Grzyb (@GrzybDev)");
	self.logger().notice("Homepage: https://grzyb.dev/project/pl_quantumbreak");
	self.logger().notice("Source code: https://github.com/GrzybDev/QuantumStreamer");
	self.logger().notice("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/QuantumStreamer/issues");
	self.logger().notice("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!");

	ServerApplication::initialize(self);
}

VOID StreamingServer::uninitialize()
{
	ServerApplication::uninitialize();
}

VOID StreamingServer::initLoggers()
{
	bool showConsole = config().getBool("Logger.ShowConsole", false);
	bool saveToLogFile = config().getBool("Logger.SaveToLogFile", false);

	if (showConsole)
		createConsole();

	// Create channels
	AutoPtr pConsoleChannel = new ConsoleChannel;
	AutoPtr pFileChannel = new FileChannel;

	std::string logFile = config().getString("Logger.LogFile", "QuantumStreamer.log");
	pFileChannel->setProperty("path", logFile);
	pFileChannel->setProperty("rotateOnOpen", "true");
	pFileChannel->setProperty("archive", "timestamp"); // archive with timestamp
	pFileChannel->setProperty("compress", "true"); // compress log files

	// Create SplitterChannel and add both channels
	AutoPtr pSplitterChannel = new SplitterChannel;
	pSplitterChannel->addChannel(pConsoleChannel);

	if (saveToLogFile)
		pSplitterChannel->addChannel(pFileChannel);

	// Create a PatternFormatter with your format
	AutoPtr pFormatter = new PatternFormatter("[%Y-%m-%d %H:%M:%S.%i][%p][%s] %t");

	// Create a FormattingChannel that wraps ConsoleChannel
	AutoPtr pFormattingChannel = new FormattingChannel(pFormatter, pSplitterChannel);

	int logLevel_Hook = config().getInt("Logger.LogLevel_Hook", Message::PRIO_FATAL);
	int logLevel_Server = config().getInt("Logger.LogLevel_Server", Message::PRIO_INFORMATION);
	int logLevel_HTTP = config().getInt("Logger.LogLevel_HTTP", Message::PRIO_FATAL);

	Logger& hookLogger = Logger::get("Hook");
	hookLogger.setChannel(pFormattingChannel);
	hookLogger.setLevel(logLevel_Hook);

	Logger& serverLogger = Logger::create("Server", pFormattingChannel, logLevel_Server);
	Logger& httpLogger = Logger::create("HTTP", pFormattingChannel, logLevel_HTTP);
}


VOID StreamingServer::createConsole()
{
	// Create a console for Debug output
	AllocConsole();

	// Redirect standard error, output to console
	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;

	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);

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


INT StreamingServer::main(const std::vector<std::string>& args)
{
	USHORT port = static_cast<USHORT>(config().getInt("Server.Port", 10000));

	Logger& logger = Logger::get("Server");
	logger.debug("Initializing HTTP Server...");

	int maxQueued = config().getInt("Server.MaxQueued", 100);
	int maxThreads = config().getInt("Server.MaxThreads", 16);
	ThreadPool::defaultPool().addCapacity(maxThreads);

	auto pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	pParams->setMaxQueued(maxQueued);
	pParams->setMaxThreads(maxThreads);
	logger.debug("Max Queued: %d", maxQueued);
	logger.debug("Max Threads: %d", maxThreads);

	// set up the server socket
	ServerSocket svs(port);
	// create the HTTP server instance
	HTTPServer srv(new RequestHandlerFactory(), svs, pParams);

	// start the server
	srv.start();

	logger.information("Started HTTP Server (Listening at port %d)", static_cast<int>(port));

	// wait for termination signal
	waitForTerminationRequest();

	logger.information("Stopping HTTP Server...");

	// stop the server
	srv.stop();

	return EXIT_OK;
}
