// Utils.cpp : Defines utility functions used throughout the application.
#include "pch.hpp"

VOID Utils::CenterPrint(std::string text, const CHAR filler, const bool endNewLine)
{
	using namespace std;

	if (!text.empty())
		text = " " + text + " ";

	CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleScreenBufferInfo);

	const UINT columns = consoleScreenBufferInfo.srWindow.Right - consoleScreenBufferInfo.srWindow.Left + 1;
	const UINT paddingSize = columns / 2 - text.size() / 2;

	PrintChar(filler, paddingSize);
	cout << text;
	PrintChar(filler, paddingSize);

	if (endNewLine)
		cout << '\n';
}

VOID Utils::PrintChar(const CHAR c, const UINT count)
{
	using namespace std;

	for (UINT i = 0; i < count; i++)
		cout << c;
}
