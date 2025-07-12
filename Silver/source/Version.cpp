#include "pch.h"
#include "../include/SDK.h"

SDK::FString SDK::FEngineVersion::ToString() const {
    std::wstring TempString =
        std::format(L"{}.{}.{}", Major, Minor, Patch);
    return TempString.c_str();
}
SDK::FString SDK::FFortniteVersion::ToString() const {
    std::wstring TempString = std::format(L"{}.{}.{}", Major, Minor, Patch);
    return TempString.c_str();
}