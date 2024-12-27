#include "pch.hpp"
#include "StreamingServer.hpp"

namespace ip = boost::asio::ip; // from <boost/asio.hpp>
using tcp = ip::tcp; // from <boost/asio.hpp>

StreamingServer::StreamingServer()
{
	InitLogging();

	BOOST_LOG_FUNCTION()
	BOOST_LOG_TRIVIAL(info) << "Initializing...";

	VideoList::GetInstance(); // Preload the video list
	SmoothStreaming::GetInstance(); // Preload the smooth streaming data
	SubtitleOverride::GetInstance(); // Preload the subtitle overrides for all episodes

	// Create an io_service object for asynchronous I/O
	boost::asio::io_service ioService;
	const auto httpClient = std::make_shared<HttpClient>(ioService);

	new StreamingServerSocket(ioService, config_->cfg->serverPort, httpClient);

	BOOST_LOG_TRIVIAL(info) << "Finished initialization, ready for receiving incoming connections!";

	ioService.run(); // Start the ioService
}

VOID StreamingServer::InitLogging() const
{
	using namespace std;
	namespace logging = boost::log;
	namespace expr = logging::expressions;

	// Get the configuration settings for logging
	const bool enableConsole = config_->cfg->debugConsole;
	const bool enableLogFile = config_->cfg->debugLogFile;
	int consoleLogLevel = config_->cfg->debugConsoleLogLevel;
	int fileLogLevel = config_->cfg->debugLogFileLevel;

	// Add common attributes for logging
	logging::add_common_attributes();
	logging::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());

	// Define the format for the log messages
	const auto logFormat = expr::format("[%1% %2%] %3%: %4%")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		% expr::format_named_scope("Scope", logging::keywords::format = "%C")
		% logging::trivial::severity
		% expr::smessage;

	if (enableConsole)
	{
		// Create a console for Debug output
		AllocConsole();

		// Redirect standard error, output to console
		// std::cout, std::clog, std::cerr, std::cin
		FILE* fDummy;

		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		cout.clear();
		clog.clear();
		cerr.clear();
		cin.clear();

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

		wcout.clear();
		wclog.clear();
		wcerr.clear();
		wcin.clear();

		// Set the filter and format for the console log
		const auto consoleSink = logging::add_console_log(std::cout);
		consoleSink->set_filter(logging::trivial::severity >= consoleLogLevel);
		consoleSink->set_formatter(logFormat);

		// Print the intro messages
		Utils::CenterPrint("Quantum Streamer by Marek Grzyb (@GrzybDev)", '=', true);
		Utils::CenterPrint("Homepage: https://grzyb.dev/project/pl_quantumbreak", ' ', false);
		Utils::CenterPrint("Source code: https://github.com/GrzybDev/QuantumStreamer", ' ', false);
		Utils::CenterPrint("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/QuantumStreamer/issues",
			' ',
			false);
		Utils::CenterPrint("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!",
			' ', true);
		Utils::CenterPrint("", '=', true);
	}

	if (enableLogFile)
	{
		// Set the filter and format for the file log
		const auto fileSink = logging::add_file_log(config_->cfg->debugLogPath);
		fileSink->set_filter(logging::trivial::severity >= fileLogLevel);
		fileSink->set_formatter(logFormat);
	}
}
