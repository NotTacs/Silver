// Silver.cpp : Defines the functions for the static library.
//

#include "pch.h"
using namespace SDK;

SDK::FUObjectArray SDK::GUObjectArray = SDK::FUObjectArray();
std::unique_ptr<MemoryLibrary> SDK::GMemLibrary = std::make_unique<MemoryLibrary>();
uintptr_t Offsets::FName_ToString = 0;
uintptr_t Offsets::GUObjectArray_ObjObjects = 0;
uintptr_t Offsets::FMemory_Realloc = 0;


bool SDK::Init() {
	AllocConsole();
	FILE* File = nullptr;
	freopen_s(&File, "CONOUT$", "w+", stdout);
	SDK::FString Str = L"You are using Silver Version 0.1. Please Update if this is not the latest version.";
	UE_LOG(LogSDK, Log, *Str);
	UE_LOG(LogSDK, Log, L"ImageBase: %p", GetModuleHandle(0));

	

	bool bChunked = true;
	GMemLibrary->FindPattern("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1");
	if (!GMemLibrary->IsValid()) {
		bChunked = false;
		/*
		* --------------------------------
		* UNCHUNKED SIGS
		* --------------------------------
		*/
		GMemLibrary->FindPattern("48 8B 05 ? ? ? ? 48 8D 14 C8 EB 03 49 8B D6 8B 42 "
			"08 C1 E8 1D A8 01 0F 85 ? ? ? ? F7 86 ? ? ? ? ? ? "
			"? ?");
		if (!GMemLibrary->IsValid()) {
			GMemLibrary->FindPattern("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? "
				"? ? 49 63 76 30");
			if (!GMemLibrary->IsValid()) {
				UE_LOG(LogMemory, Log, L"Failed to find GObjectsAddress with either Chunked Sigs or unchunked Sigs. Please report this to NotTacs on discord with version information.");
				return false;
			}
		}
	}
	Offsets::GUObjectArray_ObjObjects = GMemLibrary->Get(3, false);
	
	UE_LOG(LogMemory, Log, L"GObjectsAddress: %p", Offsets::GUObjectArray_ObjObjects);

	GUObjectArray = FUObjectArray(reinterpret_cast<void*>(Offsets::GUObjectArray_ObjObjects), 
		bChunked);

	/*
	* -----------------------
	* FMemory::Realloc Finder
	* -----------------------
	*/
	GMemLibrary->FindStringRef(L"LogCountedInstances");
	GMemLibrary->ScanFor({ 0xE8 });
	Offsets::FMemory_Realloc = GMemLibrary->Get(1);


	GMemLibrary->FindPattern("E8 ? ? ? ? 83 7C 24 ? ? 48 8D 3D ? ? ? ? 48 8B EF 48 8D 8E");
	if (!GMemLibrary->IsValid()) {
		GMemLibrary->FindStringRef(L"Material: '%s'");
		if (!GMemLibrary->IsValid()) {
			UE_LOG(LogMemory, Log, L"Failed to find StringRef for FName::ToString");
			return false;
		}
		GMemLibrary->ScanFor({ 0xE8 }, false);
	}
	Offsets::FName_ToString = GMemLibrary->Get(1);

	UE_LOG(LogMemory, Log, L"FName::ToString Address: %p", Offsets::FName_ToString);

	UE_LOG(LogMemory, Log, L"GObjectArrayNum: %d", GUObjectArray.GetObjectArrayNum());

	static std::wofstream Stream("GObjectsDump.log");
	for (int i = 0; i < GUObjectArray.GetObjectArrayNum(); i++)
	{
		FUObjectItem* Item = GUObjectArray.IndexToObject(i);
		if (!Item) continue;
		SDK::UObjectBase* Object = Item->Object;
		if (!Object)
			continue;
		Stream << *Object->GetFName().ToString() << L"\n";
	}
	Stream.close();

	return true;
}