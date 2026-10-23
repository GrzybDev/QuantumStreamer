#pragma once

#include <exception>
#include <string>

#include <Poco/Exception.h>

namespace qs
{
	/// Describes an error for the logs, with the details Poco adds to its own exceptions.
	/// @param error The error to describe.
	inline std::string errorText(const std::exception& error)
	{
		if (const auto* pocoError = dynamic_cast<const Poco::Exception*>(&error))
			return pocoError->displayText();

		return error.what();
	}
}
