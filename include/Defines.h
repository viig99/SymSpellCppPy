//
// Created by vigi99 on 27/09/20.
//

#pragma once

#ifdef _WIN32
#    ifdef LIBRARY_EXPORTS
#        define LIBRARY_API __declspec(dllexport)
#    else
#        define LIBRARY_API __declspec(dllimport)
#    endif
#endif

#ifdef UNICODE_SUPPORT
#	define xstring std::wstring
#	define xchar wchar_t
#	define xifstream std::wifstream
#	define xstringstream std::wstringstream
#	define isxspace std::iswspace
#	define xregex std::wregex
#	define xsmatch std::wsmatch
#	define to_xstring std::to_wstring
#	define XL(x) L##x
#	define xcout std::wcout
#   define to_xlower ::towlower
#   define to_xupper ::towupper
#   define is_xupper std::iswupper
#   define is_xpunct std::iswpunct
#else
#	define xstring std::string
#	define xchar char
#	define xifstream std::ifstream
#	define xstringstream std::stringstream
#	define isxspace std::isspace
#	define xregex std::regex
#	define xsmatch std::smatch
#	define to_xstring std::to_string
#	define XL(x) x
#	define xcout std::cout
#   define to_xlower ::tolower
#   define to_xupper ::toupper
#   define is_xupper std::isupper
#   define is_xpunct std::ispunct
#endif
