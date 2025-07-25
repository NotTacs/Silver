// Silver.cpp : Defines the functions for the static library.
//

#include "pch.h"
using namespace SDK;

SDK::FUObjectArray SDK::GUObjectArray = SDK::FUObjectArray();
std::unique_ptr<MemoryLibrary> SDK::GMemLibrary = std::make_unique<MemoryLibrary>();
uintptr_t Offsets::FName_ToString = 0;
uintptr_t Offsets::GUObjectArray_ObjObjects = 0;
uintptr_t Offsets::FMemory_Realloc = 0;
uintptr_t Offsets::UObject_ProcessEvent = 0;
uint32_t Offsets::Members::UFunction_Exec = -1;
FEngineVersion SDK::Engine_Version = FEngineVersion();
FFortniteVersion SDK::Fortnite_Version = FFortniteVersion();

uint32_t Offsets::Members::UStruct__SuperStruct = -1;
uint32_t Offsets::Members::UStruct__Children = -1;
uint32_t Offsets::Members::UStruct__ChildProperties = -1;
uint32_t Offsets::Members::UStruct__Size = -1;
uint32_t Offsets::Members::UStruct__MinAlignment = -1;
uint32_t Offsets::Members::UStruct__Script = -1;
uint32_t Offsets::Members::UStruct__PropertyLink = -1;
uint32_t Offsets::Members::UStruct__RefLink = -1;
uint32_t Offsets::Members::UStruct__DestructorLink = -1;
uint32_t Offsets::Members::UStruct__PostConstructorLink = -1;
uint32_t Offsets::Members::UStruct__ScriptAndPropertyObjectReferences = -1;


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

	GMemLibrary->FindPattern("FF 95 ? ? ? ? 48 8B 6C 24");
	if (!GMemLibrary->IsValid()) {
		GMemLibrary->FindPattern("FF 97 ? ? ? ? 48 8B 6C 24");
		if (!GMemLibrary->IsValid())
		{
			GMemLibrary->FindPattern("FF 95 ? ? ? ? 48 8B 6C 24");
		}
	}
	Offsets::Members::UFunction_Exec = *Memcury::Scanner(GMemLibrary->Get()).AbsoluteOffset(2).GetAs<int*>();
	UE_LOG(LogMemory, Log, L"UFunction::Exec: %d", Offsets::Members::UFunction_Exec);

	UFunction* GetEngineVersionFN = UFunction::FromName(L"GetEngineVersion");
	uint64_t NativeFuncAddress = uint64_t(GetEngineVersionFN->GetNativeFunc());
	GMemLibrary->SetAddress(NativeFuncAddress);
	GMemLibrary->ScanFor({ 0xE8 });
	static FString& (*GetEngineVersion)(FString & resstr) = decltype(GetEngineVersion)(GMemLibrary->Get(1));
	FString TempString = GetEngineVersion(TempString);

	UE_LOG(LogSDK, Log, L"BuildInfo: %s", *TempString);
	std::vector<std::wstring> TempArray;
	FString Delim = L"-";
	TempString.ParseIntoArray(TempArray, Delim);

	for (auto& Part : TempArray) {
		UE_LOG(LogSDK, Verbose, L"Part: %s", Part.c_str());
	}
	Engine_Version = TempArray[0];
	Fortnite_Version = TempArray[2];

	UE_LOG(LogSDK, Log, L"Engine_Version: %s", *Engine_Version.ToString());
	UE_LOG(LogSDK, Log, L"Fortnite_Version: %s", *Fortnite_Version.ToString());

	/*
	* ------------------------
	* BEGIN MemberOffsets
	* ------------------------
	*/
	Offsets::Members::UStruct__SuperStruct =
		Engine_Version >= FEngineVersion(4, 22, 0) ? 0x40 : 0x30; /*adds support for the class FStructBaseChain introduced in 4.22*/
	Offsets::Members::UStruct__Children =
		Offsets::Members::UStruct__SuperStruct + sizeof(void*);
	Offsets::Members::UStruct__ChildProperties = Fortnite_Version >= FFortniteVersion(12, 10, 0)
		? Offsets::Members::UStruct__Children + sizeof(void*)
		: -1; /*adds support for ChildrenProperties which is introduced at some point idk, could be introduced in 4.25 i believe*/
	Offsets::Members::UStruct__Size =
		Offsets::Members::UStruct__ChildProperties != -1
		? Offsets::Members::UStruct__ChildProperties + sizeof(void*)
		: Offsets::Members::UStruct__Children + sizeof(void*);
	Offsets::Members::UStruct__MinAlignment =
		Offsets::Members::UStruct__Size + sizeof(int32_t);
	Offsets::Members::UStruct__Script =
		Offsets::Members::UStruct__MinAlignment + sizeof(int32_t);
	Offsets::Members::UStruct__PropertyLink =
		Offsets::Members::UStruct__Script + sizeof(TArray<uint8_t>);
	Offsets::Members::UStruct__RefLink =
		Offsets::Members::UStruct__PropertyLink + sizeof(void*);
	Offsets::Members::UStruct__DestructorLink =
		Offsets::Members::UStruct__RefLink + sizeof(void*);
	Offsets::Members::UStruct__PostConstructorLink =
		Offsets::Members::UStruct__DestructorLink + sizeof(void*);
	Offsets::Members::UStruct__ScriptAndPropertyObjectReferences =
		Offsets::Members::UStruct__PostConstructorLink + sizeof(void*);

	GMemLibrary->FindPattern("41 FF 92 ? ? ? ? F6 C3");
	GMemLibrary->ScanFor({ 0x40, 0x55 }, false);
	Offsets::UObject_ProcessEvent = GMemLibrary->Get();

	return true;
}