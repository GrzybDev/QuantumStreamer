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
	Config* config_ = &Config::GetInstance();

	VOID InitLogging() const;
};
