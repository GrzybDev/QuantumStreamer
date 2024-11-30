#pragma once

#include "pch.hpp"

VOID StartServer();

class StreamingServer
{
public:
	StreamingServer();

	static StreamingServer& GetInstance()
	{
		static StreamingServer* instance;

		if (instance == nullptr)
			instance = new StreamingServer();

		return *instance;
	}

private:
	BOOL enableConsole = FALSE;
	BOOL enableLogFile = FALSE;
	USHORT streamingPort = DEFAULT_HTTP_PORT;

	VOID InitLogging() const;
};
