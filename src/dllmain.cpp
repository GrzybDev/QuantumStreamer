#include "pch.hpp"

#include "app/logging.hpp"
#include "app/quantum_streamer.hpp"

namespace
{
	DWORD WINAPI runServer(LPVOID /*parameter*/)
	{
		try
		{
			qs::QuantumStreamer application;
			application.init({"QuantumStreamer"});
			return static_cast<DWORD>(application.run());
		}
		catch (const Poco::Exception& error)
		{
			Poco::Logger::get(qs::channel::Core).fatal(error.displayText());
		}
		catch (const std::exception& error)
		{
			Poco::Logger::get(qs::channel::Core).fatal(error.what());
		}

		return EXIT_FAILURE;
	}

	std::string describeSystemError(const DWORD code)
	{
		std::array<char, 512> buffer{};
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
		               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer.data(), static_cast<DWORD>(buffer.size()),
		               nullptr);

		std::string message(buffer.data());
		message.erase(message.find_last_not_of("\r\n") + 1);
		return message;
	}
}

BOOL APIENTRY DllMain(HMODULE /*module*/, const DWORD reason, LPVOID /*reserved*/) // NOLINT(misc-use-internal-linkage)
{
	// Thread notifications stay enabled, since the static CRT needs them.
	if (reason != DLL_PROCESS_ATTACH)
		return TRUE;

	// DllMain runs under the loader lock, so the server has to start on its own thread.
	if (const HANDLE thread = CreateThread(nullptr, 0, runServer, nullptr, 0, nullptr))
	{
		CloseHandle(thread);
		return TRUE;
	}

	const DWORD error = GetLastError();
	const std::string message = std::format(
		"Quantum Streamer was unable to initialize because the worker thread could not be created.\n\n"
		"The game will now exit because critical components could not be initialized.\n"
		"Please try restarting the game, and if the problem persists, open an issue at:\n"
		"https://github.com/GrzybDev/QuantumStreamer/issues/new\n\n"
		"Please include following information:\n"
		"Error: {} ({})", error, describeSystemError(error));

	MessageBoxA(nullptr, message.c_str(), "Catastrophic Failure", MB_OK | MB_ICONERROR);
	return FALSE;
}
