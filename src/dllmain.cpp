// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"

#include "server/main.hpp"

namespace
{
	bool LaunchApp()
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
		LaunchApp();

	return TRUE;
}
