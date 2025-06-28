// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"

#include "server/main.hpp"

namespace
{
	DWORD WINAPI ServerThread(__in LPVOID /*lpParameter*/)
	{
		const char* argv[] = {"QuantumStreamer"};
		constexpr int argc = 1;

		QuantumStreamer server;
		return server.run(argc, const_cast<char**>(argv));
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, // NOLINT(misc-use-internal-linkage)
                      const DWORD ul_reason_for_call,
                      LPVOID /*lpReserved*/
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		const HANDLE hThread = CreateThread(
			nullptr, // default security attributes
			0, // use default stack size
			ServerThread, // thread function name
			nullptr, // argument to thread function
			0, // use default creation flags
			nullptr // returns the thread identifier
		);

		if (hThread == nullptr)
		{
			const DWORD errorCode = GetLastError();

			// Get the error message
			char errorBuffer[512] = {};
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				errorBuffer,
				sizeof(errorBuffer),
				nullptr
			);

			// Remove trailing newline characters
			std::string systemMessage = errorBuffer;
			systemMessage.erase(systemMessage.find_last_not_of("\r\n") + 1);

			const std::string errorMessage = std::format(
				"Quantum Streamer was unable to initialize because the worker thread could not be created.\n\n"
				"The game will now exit because critical components could not be initialized.\n"
				"Please try restarting the game, and if the problem persists, open an issue at:\n"
				"https://github.com/GrzybDev/QuantumStreamer/issues/new\n\n"
				"Please include following information:\n"
				"Error: {} ({})",
				errorCode, systemMessage);

			MessageBoxA(nullptr,
			            errorMessage.c_str(),
			            "Catastrophic Failure",
			            MB_OK | MB_ICONERROR);

			return FALSE;
		}
	}

	return TRUE;
}
