#pragma once

static constexpr unsigned char VERSION_MAJOR = 0;
static constexpr unsigned char VERSION_MINOR = 1;
static constexpr unsigned char VERSION_PATCH = 0;

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>

// Poco Header Files
#include <Poco/Util/ServerApplication.h>
