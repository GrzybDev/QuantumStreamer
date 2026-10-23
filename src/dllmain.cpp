#include "pch.hpp"

#include "app/quantum_streamer.hpp"
#include "util/errors.hpp"
#include "util/log_channels.hpp"

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
		catch (const std::exception& error)
		{
			Poco::Logger::get(qs::channel::Core).fatal(qs::errorText(error));
		}

		return EXIT_FAILURE;
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

	// User32 must not be used under the loader lock, so the loader reports the failed start instead of a dialog.
	OutputDebugStringA(std::format("Quantum Streamer could not create its server thread (error {})", GetLastError())
	                   .c_str());
	return FALSE;
}
