#include "pch.hpp"
#include "StreamingServer.hpp"

namespace ip = boost::asio::ip; // from <boost/asio.hpp>
using tcp = ip::tcp; // from <boost/asio.hpp>

StreamingServer::StreamingServer()
{
	enableConsole = boost::filesystem::exists("debugConsole");
	enableLogFile = boost::filesystem::exists("debugLogFile");

	if (boost::filesystem::exists("streamingPort"))
	{
		std::ifstream fileStream("streamingPort");
		fileStream >> streamingPort;
		fileStream.close();
	}

	InitLogging();

	BOOST_LOG_FUNCTION()
	BOOST_LOG_TRIVIAL(info) << "Initializing...";

	// Create an io_service object for asynchronous I/O
	boost::asio::io_service ioService;
	const auto httpClient = std::make_shared<HttpClient>(ioService);

	new StreamingServerSocket(ioService, streamingPort, httpClient);

	BOOST_LOG_TRIVIAL(info) << "Finished initialization, ready for receiving incoming connections!";

	ioService.run(); // Start the ioService
}

VOID StreamingServer::InitLogging() const
{
	using namespace std;
	namespace logging = boost::log;
	namespace expr = logging::expressions;

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
		consoleSink->set_filter(logging::trivial::severity >= boost::log::trivial::debug);
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
		const boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
		const auto facet = new boost::posix_time::time_facet("%Y-%m-%d_%H-%M-%S");

		std::ostringstream is;
		is.imbue(std::locale(is.getloc(), facet));
		is << timeLocal;

		// Set the filter and format for the file log
		const auto fileSink = logging::add_file_log((boost::format("qb_%1%.log") % is.str()).str());
		fileSink->set_filter(logging::trivial::severity >= boost::log::trivial::debug);
		fileSink->set_formatter(logFormat);
	}
}
