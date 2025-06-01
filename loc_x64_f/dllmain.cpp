// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"
#include "StreamingServer.hpp"

DWORD WINAPI Main(__in LPVOID lpParameter)
{
	const char* argv[] = {"StreamingServer"};
	constexpr int argc = 1;

	StreamingServer server;
	return server.run(argc, const_cast<char**>(argv));
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		// Create a thread, which will run the main function of the DLL

		if (const HANDLE hThread = CreateThread(nullptr, 0, Main, nullptr, 0, nullptr); hThread == nullptr)
		{
			// If the thread creation failed, return FALSE to indicate failure
			return FALSE;
		}
	}

	return TRUE;
}
