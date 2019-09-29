#include "stdafx.h"

#include <vector>
#include <string>
#include <sstream>
#include <array>

EXTERN_C BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

#if 0
struct ResolvedDllEntry {
	const char * const baseDllName;
	const char * const functionName;

	HMODULE moduleBase;
	FARPROC resolvedProc;
};

#define DLL_ENTRY(dll_name, function_name) \
	{ dll_name, function_name, NULL, NULL }
#define MSVCRT_ENTRY(function_name) DLL_ENTRY("msvcrt.dll", function_name)

static struct ResolvedDllEntry resolved_smybols[] = {
	MSVCRT_ENTRY("_errno"),
	MSVCRT_ENTRY("malloc"), MSVCRT_ENTRY("free"), MSVCRT_ENTRY("_callnewh"),
	MSVCRT_ENTRY("_invalid_parameter_noinfo_noreturn"),
	MSVCRT_ENTRY("abort"), MSVCRT_ENTRY("calloc"), MSVCRT_ENTRY("frexp"),
	MSVCRT_ENTRY("islower"), MSVCRT_ENTRY("isspace"), MSVCRT_ENTRY("isupper"),
	MSVCRT_ENTRY("tolower"),
	MSVCRT_ENTRY("ldexp"), MSVCRT_ENTRY("localeconv"), MSVCRT_ENTRY("__pctype_func"),
	MSVCRT_ENTRY("___lc_locale_name_func"), MSVCRT_ENTRY("___lc_codepage_func"),
	MSVCRT_ENTRY("setlocale"),
	MSVCRT_ENTRY("_wcsdup"), MSVCRT_ENTRY("wcslen"), MSVCRT_ENTRY("wcsnlen")
};
static const SIZE_T resolved_symbols_size =
sizeof(resolved_smybols) / sizeof(resolved_smybols[0]);

enum SymbolIndex {
	SYM_ERRNO,
	SYM_MALLOC, SYM_FREE, SYM_CALLNEWH,
	SYM_INVALID_PARAMETER_NOINFO_NORETURN,
	SYM_ABORT, SYM_CALLOC, SYM_FREXP,
	SYM_ISLOWER, SYM_ISSPACE, SYM_ISUPPER,
	SYM_TOLOWER,
	SYM_LDEXP, SYM_LOCALECONV, SYM_PCTYPE,
	SYM_LC_LOCALE_NAME, SYM_LC_CODEPAGE,
	SYM_SETLOCALE,
	SYM_WCSDUP, SYM_WCSLEN, SYM_WCSNLEN,
	NUMBER_OF_SYMBOLS
};

static_assert(NUMBER_OF_SYMBOLS == resolved_symbols_size, "Invalid number of Symbols in the table/enum");

#define WRAPPER_FUNCTION(symbol_index, linker_function_name, return_type, ...) \
	typedef return_type (* symbol_index ## _FN)(__VA_ARGS__); \
	extern "C" \
	return_type linker_function_name(__VA_ARGS__)
#define RUN_REAL_FN(symbol_index, ...) \
	(((symbol_index ## _FN)resolved_smybols[symbol_index].resolvedProc)(__VA_ARGS__))

int* __cdecl _errno(void) {
	typedef int*(*SYM_ERRNO_FN)();
	return (((SYM_ERRNO_FN)resolved_smybols[SYM_ERRNO].resolvedProc)());
}
WRAPPER_FUNCTION(SYM_MALLOC, malloc, void *, size_t n) {
	return RUN_REAL_FN(SYM_MALLOC, n);
}
WRAPPER_FUNCTION(SYM_FREE, free, void, void *p) {
	RUN_REAL_FN(SYM_FREE, p);
}
WRAPPER_FUNCTION(SYM_CALLNEWH, _callnewh, int, size_t n) {
	return RUN_REAL_FN(SYM_CALLNEWH, n);
}
WRAPPER_FUNCTION(SYM_INVALID_PARAMETER_NOINFO_NORETURN,
	_invalid_parameter_noinfo_noreturn, void, void) {
	ExitProcess(1);
}
WRAPPER_FUNCTION(SYM_ABORT, abort, void, void) {
	RUN_REAL_FN(SYM_ABORT);
}
WRAPPER_FUNCTION(SYM_CALLOC, calloc, void *, size_t n, size_t s) {
	return RUN_REAL_FN(SYM_CALLOC, n, s);
}
WRAPPER_FUNCTION(SYM_FREXP, frexp, double, double x, int *expptr) {
	return RUN_REAL_FN(SYM_FREXP, x, expptr);
}
WRAPPER_FUNCTION(SYM_ISLOWER, islower, int, int c) {
	return RUN_REAL_FN(SYM_ISLOWER, c);
}
WRAPPER_FUNCTION(SYM_ISSPACE, isspace, int, int c) {
	return RUN_REAL_FN(SYM_ISSPACE, c);
}
WRAPPER_FUNCTION(SYM_ISUPPER, isupper, int, int c) {
	return RUN_REAL_FN(SYM_ISUPPER, c);
}
WRAPPER_FUNCTION(SYM_TOLOWER, tolower, int, int c) {
	return RUN_REAL_FN(SYM_TOLOWER, c);
}
WRAPPER_FUNCTION(SYM_LDEXP, ldexp, double, double x, int exp) {
	return RUN_REAL_FN(SYM_LDEXP, x, exp);
}
WRAPPER_FUNCTION(SYM_LOCALECONV, localeconv, struct lconv *, void) {
	return RUN_REAL_FN(SYM_LOCALECONV);
}
WRAPPER_FUNCTION(SYM_PCTYPE, __pctype_func, const unsigned short *, void) {
	return RUN_REAL_FN(SYM_PCTYPE);
}
WRAPPER_FUNCTION(SYM_LC_LOCALE_NAME, ___lc_locale_name_func, wchar_t **, void) {
	return RUN_REAL_FN(SYM_LC_LOCALE_NAME);
}
WRAPPER_FUNCTION(SYM_LC_CODEPAGE, ___lc_codepage_func, UINT, void) {
	return RUN_REAL_FN(SYM_LC_CODEPAGE);
}
WRAPPER_FUNCTION(SYM_SETLOCALE, setlocale, char *, int category, const char *locale) {
	return RUN_REAL_FN(SYM_SETLOCALE, category, locale);
}
WRAPPER_FUNCTION(SYM_WCSDUP, _wcsdup, wchar_t *, const wchar_t *src) {
	return RUN_REAL_FN(SYM_WCSDUP, src);
}
WRAPPER_FUNCTION(SYM_WCSLEN, _wcslen, size_t, const wchar_t *str) {
	return RUN_REAL_FN(SYM_WCSLEN, str);
}
WRAPPER_FUNCTION(SYM_WCSNLEN, wcsnlen, size_t, const wchar_t *str, size_t n) {
	return RUN_REAL_FN(SYM_WCSNLEN, str, n);
}

extern "C"
void __vcrt_initialize() {}
extern "C"
void __vcrt_uninitialize() {}
extern "C"
void __vcrt_uninitialize_critical() {}
extern "C"
void __vcrt_thread_attach() {}
extern "C"
void __vcrt_thread_detach() {}
extern "C"
void __acrt_initialize() {}
extern "C"
void __acrt_uninitialize() {}
extern "C"
void __acrt_uninitialize_critical() {}
extern "C"
void __acrt_thread_attach() {}
extern "C"
void __acrt_thread_detach() {}

extern "C"
static bool resolve_all_symbols(void) {
	bool result = true;

	for (SIZE_T i = 0; i < 3; ++i) {
		if (resolved_smybols[i].moduleBase) {
			result = false;
		}
		resolved_smybols[i].moduleBase = LoadLibraryA(resolved_smybols[i].baseDllName);
		if (!resolved_smybols[i].moduleBase) {
			result = false;
			continue;
		}
		if (resolved_smybols[i].resolvedProc) {
			result = false;
		}
		resolved_smybols[i].resolvedProc = GetProcAddress(resolved_smybols[i].moduleBase,
			resolved_smybols[i].functionName);
		if (!resolved_smybols[i].resolvedProc) {
			result = false;
		}
	}

	return result;
}
#endif

void APIENTRY LibEntry(PVOID user_ptr)
{
	static bool firstEntry = true;

	if (firstEntry) {
		firstEntry = false;
#if 0
		if (!resolve_all_symbols()) {
			MessageBoxA(NULL,
				"COULD NOT RESOLVE ALL DYNAMIC DLL SYMBOLS !!!",
				"TestDLL Notification",
				MB_OK | MB_ICONINFORMATION);
			return;
		}
		void *bla = malloc(10);
		free(bla);
#endif
#if 1
		HINSTANCE addr = GetModuleHandle(NULL);
		_CRT_INIT(addr, DLL_PROCESS_ATTACH, NULL);
		std::string text;
		std::vector<DWORD> blubb;
		text = "DllMain from TestDLL: ";
		blubb.push_back(1);
		blubb.push_back(2);
		//std::stringstream muh;
		//muh << "bla" << "," << "blubb";
		MessageBoxA(NULL,
			text.c_str(),
			"TestDLL Notification",
			MB_OK | MB_ICONINFORMATION);
		char buf[128];
		snprintf(buf, sizeof buf, "_%s_\n", "bla");
		MessageBoxA(NULL,
			buf,
			"TestDLL Notification",
			MB_OK | MB_ICONINFORMATION);
#else
		MessageBoxA(NULL,
			"TEST !!!",
			"TestDLL Notification",
			MB_OK | MB_ICONINFORMATION);
#endif
	}
}