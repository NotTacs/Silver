#pragma once

namespace SDK
{
	inline std::wstring GetCurrentTime() {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()) %
			1000;
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm timeinfo{};
		localtime_s(&timeinfo, &now_c);
		std::wostringstream oss;
		oss << std::put_time(&timeinfo, L"%Y.%m.%d-%H.%M.%S") << ':'
			<< std::setfill(L'0') << std::setw(3) << ms.count();
		return oss.str();
	}
	inline void Log_Internal(FLogCategory Category, ELogVerbosity::Type Verbosity, const wchar_t* Format, ...) {
		if ((int)Verbosity > (int)(Category.GetCompileTimeVerbosity()))
			return;

		wchar_t wbuffer[2048];
		va_list args;
		va_start(args, Format);
		vswprintf(wbuffer, sizeof(wbuffer) / sizeof(wchar_t), Format, args);
		va_end(args);
		std::wstring CategoryName = Category.GetCategoryName();
		std::wcout << L"[" << GetCurrentTime() << L"]" 
			<< CategoryName << L": " << ToString(Verbosity) 
			<< L": " << wbuffer << L"\n";
	}
}

#define DEFINE_LOG_CATEGORY(CategoryName, CompileTimeVerbosity) \
inline std::string TempString_##CategoryName = std::string(#CategoryName); \
inline std::wstring TempWString_##CategoryName = std::wstring(TempString_##CategoryName.begin(), TempString_##CategoryName.end()); \
inline SDK::FLogCategory LogCategory_##CategoryName(TempWString_##CategoryName, SDK::ELogVerbosity::CompileTimeVerbosity);

#define UE_LOG(CategoryName, Verbosity, Format, ...) \
SDK::Log_Internal(LogCategory_##CategoryName, SDK::ELogVerbosity::##Verbosity, Format, ##__VA_ARGS__)