// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"

#include "StreamingServer.hpp"

extern "C" uintptr_t ImportedFunctions[73] = {};
HMODULE OriginalLibrary = nullptr;

std::vector<const char*> BinkFunctions =
{
	"BinkAllocateFrameBuffers",
	"BinkClose",
	"BinkCloseTrack",
	"BinkControlBackgroundIO",
	"BinkCopyToBuffer",
	"BinkCopyToBufferRect",
	"BinkDoFrame",
	"BinkDoFrameAsync",
	"BinkDoFrameAsyncMulti",
	"BinkDoFrameAsyncWait",
	"BinkDoFramePlane",
	"BinkFreeGlobals",
	"BinkGetError",
	"BinkGetFrameBuffersInfo",
	"BinkGetGPUDataBuffersInfo",
	"BinkGetKeyFrame",
	"BinkGetPlatformInfo",
	"BinkGetRealtime",
	"BinkGetRects",
	"BinkGetSummary",
	"BinkGetTrackData",
	"BinkGetTrackID",
	"BinkGetTrackMaxSize",
	"BinkGetTrackType",
	"BinkGoto",
	"BinkLogoAddress",
	"BinkNextFrame",
	"BinkOpen",
	"BinkOpenDirectSound",
	"BinkOpenMiles",
	"BinkOpenTrack",
	"BinkOpenWaveOut",
	"BinkOpenWithOptions",
	"BinkOpenXAudio2",
	"BinkOpenXAudio27",
	"BinkOpenXAudio28",
	"BinkPause",
	"BinkRegisterFrameBuffers",
	"BinkRegisterGPUDataBuffers",
	"BinkRequestStopAsyncThread",
	"BinkRequestStopAsyncThreadsMulti",
	"BinkService",
	"BinkSetError",
	"BinkSetFileOffset",
	"BinkSetFrameRate",
	"BinkSetIO",
	"BinkSetIOSize",
	"BinkSetMemory",
	"BinkSetOSFileCallbacks",
	"BinkSetPan",
	"BinkSetSimulate",
	"BinkSetSoundOnOff",
	"BinkSetSoundSystem",
	"BinkSetSoundSystem2",
	"BinkSetSoundTrack",
	"BinkSetSpeakerVolumes",
	"BinkSetVideoOnOff",
	"BinkSetVolume",
	"BinkSetWillLoop",
	"BinkShouldSkip",
	"BinkStartAsyncThread",
	"BinkUtilCPUs",
	"BinkUtilFree",
	"BinkUtilMalloc",
	"BinkUtilMutexCreate",
	"BinkUtilMutexDestroy",
	"BinkUtilMutexLock",
	"BinkUtilMutexLockTimeOut",
	"BinkUtilMutexUnlock",
	"BinkWait",
	"BinkWaitStopAsyncThread",
	"BinkWaitStopAsyncThreadsMulti",
	"RADTimerRead"
};

VOID LoadOriginalLibrary(const LPCSTR libraryName)
{
	// Load the original library
	OriginalLibrary = LoadLibraryA(libraryName);

	if (OriginalLibrary == nullptr)
	{
		MessageBoxA(nullptr, "Failed to load bink2w64_original.dll! Cannot continue.", "Failed to initialize hook!",
		            MB_ICONERROR);
		ExitProcess(DLL_LOAD_FAILURE);
	}

	for (size_t i = 0; i < BinkFunctions.size(); i++)
		ImportedFunctions[i] = reinterpret_cast<uintptr_t>(GetProcAddress(OriginalLibrary, BinkFunctions[i]));
}

DWORD WINAPI InitServer(LPVOID /*lpParam*/)
{
	StreamingServer::GetInstance();

	return NULL;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/,
                      const DWORD ulReasonForCall,
                      LPVOID /*lpReserved*/
)
{
	// If the DLL is being loaded, load the original library and create a new thread to initialize the hook
	if (ulReasonForCall == DLL_PROCESS_ATTACH)
	{
		LoadOriginalLibrary("bink2w64_original.dll");

		CreateThread(nullptr, NULL, InitServer, nullptr, NULL, nullptr);
	}

	return TRUE;
}
