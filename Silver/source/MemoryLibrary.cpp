#include "pch.h"
#include "../include/SDK.h"
using namespace Memcury;

void SDK::MemoryLibrary::FindPattern(const std::string& Pattern) {
	Scanner = Scanner::FindPattern(Pattern.c_str());
}
void SDK::MemoryLibrary::FindStringRef(const std::wstring& StringRef) {
	Scanner = Scanner::FindStringRef(StringRef.c_str());
}