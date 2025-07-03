#pragma once

static constexpr unsigned char VERSION_MAJOR = 0;
static constexpr unsigned char VERSION_MINOR = 1;
static constexpr unsigned char VERSION_PATCH = 0;

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Standard C++ Header Files
#include <format>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Windows Header Files
#include <tchar.h>
#include <windows.h>

// Poco Header Files
#include <Poco/AutoPtr.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/PatternFormatter.h>
#include <Poco/SplitterChannel.h>
#include <Poco/ThreadPool.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/ServerApplication.h>
