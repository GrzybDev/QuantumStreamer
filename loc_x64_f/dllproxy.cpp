#include "pch.hpp"

using Poco::Exception;
using Poco::Logger;
using Poco::Util::Application;

static Logger& logger = Logger::get("Hook");

// Original DLL handle
HMODULE originalDll = nullptr;

// Variable declarations for function pointers
void* real_sm_codeLocaleDictionary = nullptr;
StringTable** real_sm_pInstance = nullptr;

// Function pointer types
t_StringTable_CopyCtor real_StringTable_CopyCtor = nullptr;
t_StringTable_DefaultCtor real_StringTable_DefaultCtor = nullptr;
t_StringTable_Dtor real_StringTable_Dtor = nullptr;
t_StringTable_Assign real_StringTable_assign = nullptr;
t_WordWrap_MoveAssign real_WordWrap_MoveAssign = nullptr;
t_WordWrap_CopyAssign real_WordWrap_CopyAssign = nullptr;
t_CanBreakLineAt real_CanBreakLineAt = nullptr;
t_FindNextLine real_FindNextLine = nullptr;
t_IsWhiteSpace real_IsWhiteSpace = nullptr;
t_SetCallback real_SetCallback = nullptr;
t_addStringTable real_addStringTable = nullptr;
t_convertToWString real_convertToWString = nullptr;
t_convertToWString_External real_convertToWString_External = nullptr;
t_fillCodeLocaleTable real_fillCodeLocaleTable = nullptr;
t_fixString real_fixString = nullptr;
t_getInstance real_getInstance = nullptr;
t_getStringById real_getStringById = nullptr;
t_load real_load = nullptr;
t_loadFromBinary real_loadFromBinary = nullptr;
t_loadFromXML_RMD_Format real_loadFromXML_RMD_Format = nullptr;
t_loadFromXml_RMD_Format real_loadFromXml_RMD_Format = nullptr;

void LoadOriginalDll()
{
	if (originalDll) return; // Already loaded

	logger.information("Loading Original DLL...");

	const Application& app = Application::instance();
	const std::string dllName = app.config().getString("Hook.OriginalLibraryName", "loc_x64_f_o.dll");

	originalDll = LoadLibraryA(dllName.c_str());

	if (!originalDll)
	{
		logger.fatal("Failed to load original DLL: '%s'. Hooking is not possible!", dllName);

		const std::string msgBoxErrorText =
			"Failed to load original game library!\nAccording to your config original loc_x64_f.dll file should be named "
			+ dllName + ", hooking is not possible without original file!\n\nGame will now exit.";
		MessageBoxA(nullptr, msgBoxErrorText.c_str(), "Quantum Streamer - Fatal Error!", MB_OK | MB_ICONERROR);

		throw Exception("Failed to load original DLL: " + dllName);
	}

	logger.information("Original DLL ('%s') has been loaded successfully!", dllName);
}

/* Below there are function implementations for all exported functions from the original DLL.
   These functions are exported to be used by the game engine, and they will
   call the corresponding functions in the original DLL.
*/

extern "C" __declspec(dllexport)
StringTable* __thiscall StringTable_CopyCtor(StringTable* _this, StringTable* other)
{
	logger.debug("StringTable (Copy Constructor) called");

	if (!real_StringTable_CopyCtor)
	{
		if (!originalDll) LoadOriginalDll();

		real_StringTable_CopyCtor = (t_StringTable_CopyCtor)(GetProcAddress(
			originalDll, "??0StringTable@loc@@QEAA@AEBV01@@Z"));
		if (!real_StringTable_CopyCtor)
		{
			logger.error("Failed to resolve copy constructor for StringTable");
			return nullptr;
		}
	}

	return real_StringTable_CopyCtor(_this, other);
}

extern "C" __declspec(dllexport)
StringTable* __thiscall StringTable_DefaultCtor(StringTable* _this)
{
	logger.debug("StringTable (Default Constructor) called");

	if (!real_StringTable_DefaultCtor)
	{
		if (!originalDll) LoadOriginalDll();

		real_StringTable_DefaultCtor = (t_StringTable_DefaultCtor)(GetProcAddress(
			originalDll, "??0StringTable@loc@@QEAA@XZ"));
		if (!real_StringTable_DefaultCtor)
		{
			logger.error("Failed to resolve default constructor for StringTable");
			return nullptr;
		}
	}

	return real_StringTable_DefaultCtor(_this);
}

extern "C" __declspec(dllexport)
void __thiscall StringTable_Dtor(StringTable* _this)
{
	logger.debug("StringTable (Destructor) called");

	if (!real_StringTable_Dtor)
	{
		if (!originalDll) LoadOriginalDll();

		real_StringTable_Dtor = (t_StringTable_Dtor)(GetProcAddress(originalDll, "??1StringTable@loc@@QEAA@XZ"));
		if (!real_StringTable_Dtor)
		{
			logger.error("Failed to resolve destructor for StringTable");
			return;
		}
	}

	real_StringTable_Dtor(_this);
}

extern "C" __declspec(dllexport)
StringTable* __thiscall StringTable_assign(StringTable* _this, const StringTable* other)
{
	logger.debug("StringTable (Copy Assignment Operator) called");

	if (!real_StringTable_assign)
	{
		if (!originalDll) LoadOriginalDll();

		real_StringTable_assign = (t_StringTable_Assign)GetProcAddress(originalDll,
		                                                               "??4StringTable@loc@@QEAAAEAV01@AEBV01@@Z");
		if (!real_StringTable_assign)
		{
			logger.error("Failed to resolve assignment operator for StringTable");
			return nullptr;
		}
	}

	return real_StringTable_assign(_this, other);
}

extern "C" __declspec(dllexport)
WordWrap* __thiscall WordWrap_MoveAssign(WordWrap* _this, WordWrap&& other)
{
	logger.debug("WordWrap (Move Assignment Operator) called");

	if (!real_WordWrap_MoveAssign)
	{
		if (!originalDll) LoadOriginalDll();

		real_WordWrap_MoveAssign = (t_WordWrap_MoveAssign)GetProcAddress(
			originalDll, "??4WordWrap@@QEAAAEAV0@$$QEAV0@@Z");
		if (!real_WordWrap_MoveAssign)
		{
			logger.error("Failed to resolve move assignment operator for WordWrap");
			return nullptr;
		}
	}

	return real_WordWrap_MoveAssign(_this, std::move(other));
}

extern "C" __declspec(dllexport)
WordWrap* __thiscall WordWrap_CopyAssign(WordWrap* _this, const WordWrap* other)
{
	logger.debug("WordWrap (Copy Assignment Operator) called");

	if (!real_WordWrap_CopyAssign)
	{
		if (!originalDll) LoadOriginalDll();

		real_WordWrap_CopyAssign = (t_WordWrap_CopyAssign)
			GetProcAddress(originalDll, "??4WordWrap@@QEAAAEAV0@AEBV0@@Z");
		if (!real_WordWrap_CopyAssign)
		{
			logger.error("Failed to resolve copy assignment operator for WordWrap");
			return nullptr;
		}
	}

	return real_WordWrap_CopyAssign(_this, other);
}

extern "C" __declspec(dllexport)
bool __cdecl CanBreakLineAt(const wchar_t* str, wchar_t ch)
{
	logger.debug("WordWrap::CanBreakLineAt called");

	if (!real_CanBreakLineAt)
	{
		if (!originalDll) LoadOriginalDll();

		real_CanBreakLineAt = (t_CanBreakLineAt)GetProcAddress(originalDll, "?CanBreakLineAt@WordWrap@@SA_NPEB_W0@Z");
		if (!real_CanBreakLineAt)
		{
			logger.error("Failed to resolve WordWrap::CanBreakLineAt");
			return false;
		}
	}

	return real_CanBreakLineAt(str, ch);
}

extern "C" __declspec(dllexport)
const wchar_t* __cdecl FindNextLine(const wchar_t* str, unsigned int len, const wchar_t** outPos)
{
	logger.debug("WordWrap::FindNextLine called");

	if (!real_FindNextLine)
	{
		if (!originalDll) LoadOriginalDll();

		real_FindNextLine = (t_FindNextLine)GetProcAddress(originalDll,
		                                                   "?FindNextLine@WordWrap@@SAPEB_WPEB_WIPEAPEB_W@Z");
		if (!real_FindNextLine)
		{
			logger.error("Failed to resolve WordWrap::FindNextLine");
			return nullptr;
		}
	}

	return real_FindNextLine(str, len, outPos);
}

extern "C" __declspec(dllexport)
bool __cdecl IsWhiteSpace(const wchar_t* ch)
{
	logger.debug("WordWrap::IsWhiteSpace called");

	if (!real_IsWhiteSpace)
	{
		if (!originalDll) LoadOriginalDll();

		real_IsWhiteSpace = (t_IsWhiteSpace)GetProcAddress(originalDll, "?IsWhiteSpace@WordWrap@@SA_NPEB_W@Z");
		if (!real_IsWhiteSpace)
		{
			logger.error("Failed to resolve WordWrap::IsWhiteSpace");
			return false;
		}
	}

	return real_IsWhiteSpace(ch);
}

extern "C" __declspec(dllexport)
void __cdecl SetCallback(int (*cb1)(wchar_t), void (*cb2)())
{
	logger.debug("WordWrap::SetCallback called");

	if (!real_SetCallback)
	{
		if (!originalDll) LoadOriginalDll();

		real_SetCallback = (t_SetCallback)GetProcAddress(originalDll, "?SetCallback@WordWrap@@SAXP6AI_W@ZP6AIXZ@Z");
		if (!real_SetCallback)
		{
			logger.error("Failed to resolve WordWrap::SetCallback");
			std::cerr << "[ERROR] Failed to resolve WordWrap::SetCallback\n";
			return;
		}
	}

	real_SetCallback(cb1, cb2);
}

extern "C" __declspec(dllexport)
void __thiscall addStringTable(StringTable* _this, Document& param_1)
{
	logger.debug("StringTable::addStringTable called");

	if (!real_addStringTable)
	{
		if (!originalDll) LoadOriginalDll();

		real_addStringTable = (t_addStringTable)GetProcAddress(originalDll,
		                                                       "?addStringTable@StringTable@loc@@QEAAXAEAVDocument@xml@@@Z");
		if (!real_addStringTable)
		{
			logger.error("Failed to resolve StringTable::addStringTable");
			return;
		}
	}

	real_addStringTable(_this, param_1);
}

extern "C" __declspec(dllexport)
void __cdecl convertToWString(InplaceString* param_1, InplaceWString* param_2)
{
	logger.debug("StringTable::convertToWString called");

	if (!real_convertToWString)
	{
		if (!originalDll) LoadOriginalDll();

		real_convertToWString = (t_convertToWString)GetProcAddress(originalDll,
		                                                           "?convertToWString@StringTable@loc@@SAXAEBV?$InplaceString@$0IA@@r@@AEAV?$InplaceWString@$0IA@@4@@Z");
		if (!real_convertToWString)
		{
			logger.error("Failed to resolve StringTable::convertToWString");
			return;
		}
	}

	real_convertToWString(param_1, param_2);
}

extern "C" __declspec(dllexport)
void __cdecl convertToWString_External(InplaceString* param_1, InplaceWString* param_2)
{
	logger.debug("StringTable::convertToWString_External called");

	if (!real_convertToWString_External)
	{
		if (!originalDll) LoadOriginalDll();

		real_convertToWString_External = (t_convertToWString_External)GetProcAddress(
			originalDll,
			"?convertToWString_External@StringTable@loc@@SAXAEBV?$InplaceString@$0IA@@r@@AEAV?$InplaceWString@$0IA@@4@@Z");
		if (!real_convertToWString_External)
		{
			logger.error("Failed to resolve StringTable::convertToWString_External");
			return;
		}
	}

	real_convertToWString_External(param_1, param_2);
}

extern "C" __declspec(dllexport)
void __cdecl fillCodeLocaleTable()
{
	logger.debug("StringTable::fillCodeLocaleTable called");

	if (!real_fillCodeLocaleTable)
	{
		if (!originalDll) LoadOriginalDll();

		real_fillCodeLocaleTable = (t_fillCodeLocaleTable)GetProcAddress(
			originalDll, "?fillCodeLocaleTable@StringTable@loc@@KAXXZ");
		if (!real_fillCodeLocaleTable)
		{
			logger.error("Failed to resolve StringTable::fillCodeLocaleTable");
			return;
		}
	}

	real_fillCodeLocaleTable();
}

extern "C" __declspec(dllexport)
void __thiscall fixString(StringTable* _this, wchar_t** param_1, char* param_2)
{
	logger.debug("StringTable::fixString called");

	if (!real_fixString)
	{
		if (!originalDll) LoadOriginalDll();

		real_fixString = (t_fixString)GetProcAddress(originalDll, "?fixString@StringTable@loc@@QEAAXAEAPEB_WPEBD@Z");
		if (!real_fixString)
		{
			logger.error("Failed to resolve StringTable::fixString");
			return;
		}
	}

	real_fixString(_this, param_1, param_2);
}

extern "C" __declspec(dllexport)
StringTable* __cdecl getInstance()
{
	if (!originalDll) LoadOriginalDll();

	logger.debug("StringTable::getInstance called");

	if (!real_getInstance)
	{
		real_getInstance = (t_getInstance)GetProcAddress(originalDll, "?getInstance@StringTable@loc@@SAPEAV12@XZ");
		if (!real_getInstance)
		{
			logger.error("Failed to resolve StringTable::getInstance");
			return nullptr;
		}
	}

	return real_getInstance();
}

extern "C" __declspec(dllexport)
wchar_t* __thiscall getStringById(StringTable* _this, char* param_1, char* param_2)
{
	logger.debug("StringTable::getStringById called");

	if (!real_getStringById)
	{
		if (!originalDll) LoadOriginalDll();

		real_getStringById = (t_getStringById)GetProcAddress(originalDll,
		                                                     "?getStringById@StringTable@loc@@QEAAPEB_WPEBD0@Z");
		if (!real_getStringById)
		{
			logger.error("Failed to resolve StringTable::getStringById");
			return nullptr;
		}
	}

	return real_getStringById(_this, param_1, param_2);
}

extern "C" __declspec(dllexport)
bool __thiscall load(void* _this, FileProvider* param_1)
{
	logger.debug("StringTable::load called");

	if (!real_load)
	{
		if (!originalDll) LoadOriginalDll();

		real_load = (t_load)GetProcAddress(originalDll, "?load@StringTable@loc@@QEAA_NPEAVFileProvider@12@@Z");
		if (!real_load)
		{
			logger.error("Failed to resolve StringTable::load");
			return false;
		}
	}

	return real_load(_this, param_1);
}

extern "C" __declspec(dllexport)
bool __cdecl loadFromBinary(const InplaceString& param_1, unsigned int param_2,
                            PseudoMap<InplaceString, InplaceWString>& param_3,
                            FileProvider* param_4)
{
	logger.debug("StringTable::loadFromBinary called");

	if (!real_loadFromBinary)
	{
		if (!originalDll) LoadOriginalDll();

		real_loadFromBinary = (t_loadFromBinary)GetProcAddress(originalDll,
		                                                       "?loadFromBinary@StringTable@loc@@SA_NPEBDAEAV?$PseudoMap@V?$InplaceString@$0IA@@r@@V?$InplaceWString@$0IA@@2@@r@@PEAVFileProvider@12@@Z");
		if (!real_loadFromBinary)
		{
			logger.error("Failed to resolve StringTable::loadFromBinary");
			return false;
		}
	}

	return real_loadFromBinary(param_1, param_2, param_3, param_4);
}

extern "C" __declspec(dllexport)
bool __cdecl loadFromXML_RMD_Format(const InplaceString& param_1, unsigned int param_2,
                                    PseudoMap<InplaceString, InplaceWString>& param_3)
{
	logger.debug("StringTable::loadFromXML_RMD_Format called");

	if (!real_loadFromXML_RMD_Format)
	{
		if (!originalDll) LoadOriginalDll();

		real_loadFromXML_RMD_Format = (t_loadFromXML_RMD_Format)GetProcAddress(
			originalDll,
			"?loadFromXML_RMD_Format@StringTable@loc@@SA_NPEBDAEAV?$PseudoMap@V?$InplaceString@$0IA@@r@@V?$InplaceWString@$0IA@@2@@r@@@Z");
		if (!real_loadFromXML_RMD_Format)
		{
			logger.error("Failed to resolve StringTable::loadFromXML_RMD_Format");
			return false;
		}
	}

	return real_loadFromXML_RMD_Format(param_1, param_2, param_3);
}

extern "C" __declspec(dllexport)
unsigned int __cdecl loadFromXml_RMD_Format(Document& param_1, const InplaceString& param_2,
                                            PseudoMap<InplaceString, InplaceWString>& param_3)
{
	logger.debug("StringTable::loadFromXml_RMD_Format called");

	if (!real_loadFromXml_RMD_Format)
	{
		if (!originalDll) LoadOriginalDll();

		real_loadFromXml_RMD_Format = (t_loadFromXml_RMD_Format)GetProcAddress(
			originalDll,
			"?loadFromXml_RMD_Format@StringTable@loc@@SA_NAEAVDocument@xml@@PEBDAEAV?$PseudoMap@V?$InplaceString@$0IA@@r@@V?$InplaceWString@$0IA@@2@@r@@@Z");
		if (!real_loadFromXml_RMD_Format)
		{
			logger.error("Failed to resolve StringTable::loadFromXml_RMD_Format");
			return 0;
		}
	}

	return real_loadFromXml_RMD_Format(param_1, param_2, param_3);
}

extern "C" __declspec(dllexport)
void* sm_codeLocaleDictionary()
{
	logger.debug("Accessing StringTable::sm_codeLocaleDictionary");

	if (!real_sm_codeLocaleDictionary)
	{
		if (!originalDll) LoadOriginalDll();

		real_sm_codeLocaleDictionary = GetProcAddress(originalDll,
		                                              "?sm_codeLocaleDictionary@StringTable@loc@@1V?$map@V?$InplaceString@$0IA@@r@@V?$map@V?$InplaceString@$0IA@@r@@V?$InplaceWString@$0IA@@2@U?$less@V?$InplaceString@$0IA@@r@@@std@@V?$Allocator@V?$InplaceWString@$0IA@@r@@@2@@std@@U?$less@V?$InplaceString@$0IA@@r@@@4@V?$Allocator@V?$map@V?$InplaceString@$0IA@@r@@V?$InplaceWString@$0IA@@2@U?$less@V?$InplaceString@$0IA@@r@@@std@@V?$Allocator@V?$InplaceWString@$0IA@@r@@@2@@std@@@2@@std@@A");
		if (!real_sm_codeLocaleDictionary)
		{
			logger.error("Failed to resolve StringTable::sm_codeLocaleDictionary");
		}
	}

	return real_sm_codeLocaleDictionary;
}

extern "C" __declspec(dllexport)
StringTable* sm_pInstance()
{
	logger.debug("Accessing StringTable::sm_pInstance");

	if (!real_sm_pInstance)
	{
		if (!originalDll) LoadOriginalDll();

		real_sm_pInstance = (StringTable**)GetProcAddress(originalDll, "?sm_pInstance@StringTable@loc@@1PEAV12@EA");
		if (!real_sm_pInstance)
		{
			logger.error("Failed to resolve StringTable::sm_pInstance");
			return nullptr;
		}
	}

	// Return the actual pointer held by sm_pInstance (which itself is a pointer to a pointer)
	return *real_sm_pInstance;
}
