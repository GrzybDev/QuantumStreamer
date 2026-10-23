#pragma once

#include "settings.hpp"

namespace qs
{
	/// Points every channel at the console and, when enabled, at a rotating log file.
	/// @param settings Where to write and how detailed each channel is.
	void setupLogging(const Settings::Logging& settings);
}
