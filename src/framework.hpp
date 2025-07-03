#pragma once

static constexpr unsigned char VERSION_MAJOR = 0;
static constexpr unsigned char VERSION_MINOR = 1;
static constexpr unsigned char VERSION_PATCH = 0;

#define BLOCK_MDAT "mdat"
#define BLOCK_MFRA "mfra"
#define BLOCK_MOOF "moof"
#define BLOCK_TFRA "tfra"

#define EPISODE_TITLE_START_TIME "80080000"

#define FLAGS_TO_SKIP 3
#define REMOTE_TIMEOUT 20 // seconds (Game seems to use 20 seconds till it tries to retry)

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Standard C++ Header Files
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

// Windows Header Files
#include <tchar.h>
#include <windows.h>

// Poco Header Files
#include <Poco/AutoPtr.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/PatternFormatter.h>
#include <Poco/SplitterChannel.h>
#include <Poco/StreamCopier.h>
#include <Poco/ThreadPool.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/MongoDB/Array.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/ServerApplication.h>
