#pragma once
namespace SDK
{
	inline void CheckHandler(const char* expr, const char* file, int line, const std::wstring& msg = L"") {
		std::cerr << "Check Failed: (" << expr << ")" << " at " << file << ":" << line;
		if (!msg.empty())
			std::wcerr << L" - " << msg;
		std::cerr << std::endl;

		std::abort();
	}
	inline void CheckSlowHandler(const wchar_t* expr, const wchar_t* file, int line) {
		std::wcerr << L"Check Failed: (" << expr << L")" << L" at " << file << L":" << line;
		std::wcerr << std::endl;
		std::abort();
	}

	inline void CheckStrHandler(const char* expr, const char* file, int line, const std::string& msg = "") {
		std::cerr << "Check Failed: (" << expr << ")" << " at " << file << ":" << line;
		if (!msg.empty())
			std::cerr << " - " << msg;
		std::cerr << std::endl;

		std::abort();
	}
}

#define DO_CHECK_SLOW _DEBUG

#if DO_CHECK_SLOW
#define checkSlow(expr) ( (expr) ) ? static_cast<void>(0) : SDK::CheckSlowHandler(L#expr, __FILEW__, __LINE__)
#else
#define checkSlow(expr) static_cast<void>(0)
#endif

#define check( expr )                                                          \
        ( ( expr ) ? static_cast<void>( 0 )                                    \
                   : SDK::CheckStrHandler( #expr, __FILE__, __LINE__ ) )

#define checkf( expr, message, ... )                                           \
        ( ( expr ) ? static_cast<void>( 0 )                                    \
                   : SDK::CheckHandler( #expr, __FILE__, __LINE__,                  \
                                   std::format( message, __VA_ARGS__ ) ) )

#define RESTRICT __restrict
#define UE_ASSUME( x ) __assume( x )