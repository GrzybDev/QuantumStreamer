// Utils.hpp : Defines the Utils class, which provides utility functions used throughout the application.
#pragma once

class Utils
{
public:
	static VOID CenterPrint(std::string text, CHAR filler, bool endNewLine = true);
	static VOID PrintChar(CHAR c, UINT count);
};
