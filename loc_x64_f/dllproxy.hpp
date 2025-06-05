#pragma once

// Handle to the original DLL and function to load it  
extern HMODULE originalDll;
void LoadOriginalDll();

// Forward declarations of classes and structs used in the proxy  
struct Document;

struct InplaceString_128
{
	char data[128];
};

using InplaceString = InplaceString_128;
using PInplaceString = InplaceString*;

struct InplaceWString_128
{
	wchar_t data[128];
};

using InplaceWString = InplaceWString_128;
using PInplaceWString = InplaceWString*;

struct FileProvider;
template <typename K, typename V>
struct PseudoMap;
struct StringTable;
struct StringTable_inplace_string;
struct StringTable_inplace_wstring;
struct WordWrap;

// Forward declarations of variables used in the proxy  
extern void* real_sm_codeLocaleDictionary;
extern StringTable** real_sm_pInstance;

// Function pointer types for the functions we will be hooking  
using t_StringTable_CopyCtor = StringTable* (__thiscall*)(StringTable* _this, StringTable* other);
using t_StringTable_DefaultCtor = StringTable* (__thiscall*)(StringTable* _this);
using t_StringTable_Dtor = void(__thiscall*)(StringTable* _this);
using t_StringTable_Assign = StringTable* (__thiscall*)(StringTable* _this, const StringTable* other);
using t_WordWrap_MoveAssign = WordWrap* (__thiscall*)(WordWrap* _this, WordWrap&& other);
using t_WordWrap_CopyAssign = WordWrap* (__thiscall*)(WordWrap* _this, const WordWrap* other);
using t_CanBreakLineAt = bool(__cdecl*)(const wchar_t* str, wchar_t ch);
using t_FindNextLine = const wchar_t* (__cdecl*)(const wchar_t* str, unsigned int len, const wchar_t** outPos);
using t_IsWhiteSpace = bool(__cdecl*)(const wchar_t* ch);
using t_SetCallback = void(__cdecl*)(int (*cb1)(wchar_t), void (*cb2)());
using t_addStringTable = void(__thiscall*)(StringTable* _this, Document& param_1);
using t_convertToWString = void(__cdecl*)(InplaceString* param_1, InplaceWString* param_2);
using t_convertToWString_External = void(__cdecl*)(InplaceString* param_1, InplaceWString* param_2);
using t_fillCodeLocaleTable = void(__cdecl*)(void);
using t_fixString = void(__thiscall*)(StringTable* _this, wchar_t** param_1, char* param_2);
using t_getInstance = StringTable* (__cdecl*)(void);
using t_getStringById = wchar_t* (__thiscall*)(StringTable* _this, char* param_1, char* param_2);
using t_load = bool(__thiscall*)(void* _this, FileProvider* param_1);
using t_loadFromBinary = bool(__cdecl*)(const InplaceString& param_1, unsigned int param_2,
                                        PseudoMap<InplaceString, InplaceWString>& param_3,
                                        FileProvider* param_4);
using t_loadFromXML_RMD_Format = bool(__cdecl*)(const InplaceString& param_1, unsigned int param_2,
                                                PseudoMap<InplaceString, InplaceWString>& param_3);
using t_loadFromXml_RMD_Format = unsigned int(__cdecl*)(Document& param_1, const InplaceString& param_2,
                                                        PseudoMap<InplaceString, InplaceWString>& param_3);

// Function pointers to the original functions  
extern t_StringTable_CopyCtor real_StringTable_CopyCtor;
extern t_StringTable_DefaultCtor real_StringTable_DefaultCtor;
extern t_StringTable_Dtor real_StringTable_Dtor;
extern t_StringTable_Assign real_StringTable_assign;
extern t_WordWrap_MoveAssign real_WordWrap_MoveAssign;
extern t_WordWrap_CopyAssign real_WordWrap_CopyAssign;
extern t_CanBreakLineAt real_CanBreakLineAt;
extern t_FindNextLine real_FindNextLine;
extern t_IsWhiteSpace real_IsWhiteSpace;
extern t_SetCallback real_SetCallback;
extern t_addStringTable real_addStringTable;
extern t_convertToWString real_convertToWString;
extern t_convertToWString_External real_convertToWString_External;
extern t_fillCodeLocaleTable real_fillCodeLocaleTable;
extern t_fixString real_fixString;
extern t_getInstance real_getInstance;
extern t_getStringById real_getStringById;
extern t_load real_load;
extern t_loadFromBinary real_loadFromBinary;
extern t_loadFromXML_RMD_Format real_loadFromXML_RMD_Format;
extern t_loadFromXml_RMD_Format real_loadFromXml_RMD_Format;
