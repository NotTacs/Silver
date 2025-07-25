#include "pch.h"
#include "../include/SDK.h"

SDK::FString SDK::FName::ToString() const {
    static FString& (*InternalToString)(const FName*, FString&) =
        decltype(InternalToString)(Offsets::FName_ToString);

    FString Ret = InternalToString(this, Ret);
    return Ret;
}

SDK::FNativeFuncPtr SDK::UFunction::Func() const {
    return *reinterpret_cast<SDK::FNativeFuncPtr*>(
        __int64(this) + SDK::Offsets::Members::UFunction_Exec);
}

void SDK::UFunction::SetNativeFunc(FNativeFuncPtr InFunc) {
    *reinterpret_cast<SDK::FNativeFuncPtr*>(
        __int64(this) + SDK::Offsets::Members::UFunction_Exec) = InFunc;
}

SET_OFFSET(SDK::UStruct*, SDK::UStruct::SuperStruct, SDK::Offsets::Members::UStruct__SuperStruct);
SET_OFFSET(SDK::UField*, SDK::UStruct::Children, SDK::Offsets::Members::UStruct__Children);
SET_OFFSET(SDK::FField*, SDK::UStruct::ChildrenProperties, SDK::Offsets::Members::UStruct__ChildProperties);
SET_OFFSET(int32, SDK::UStruct::Size, SDK::Offsets::Members::UStruct__Size);
SET_OFFSET(int32, SDK::UStruct::MinAlignment, SDK::Offsets::Members::UStruct__MinAlignment);
SDK::TArray<uint8>& SDK::UStruct::Script() const {
    return *reinterpret_cast<SDK::TArray<uint8> *>(
        reinterpret_cast<uintptr_t>(this) +
        SDK::Offsets::Members::UStruct__Script);
};
SET_OFFSET(void*, SDK::UStruct::PropertyLink, SDK::Offsets::Members::UStruct__PropertyLink);
SET_OFFSET(void*, SDK::UStruct::RefLink, SDK::Offsets::Members::UStruct__RefLink);
SET_OFFSET(void*, SDK::UStruct::DestructorLink, SDK::Offsets::Members::UStruct__DestructorLink);
SET_OFFSET(void*, SDK::UStruct::PostConstructorLink, SDK::Offsets::Members::UStruct__PostConstructorLink);

SDK::UClass* SDK::StaticClassImpl(const char* ClassName) {
    static std::unordered_map<std::string, SDK::UClass*> ClassCache;
    auto It = ClassCache.find(ClassName);
    if (It != ClassCache.end()) {
        return It->second;
    }
    const std::string ClassNameStr = std::string(ClassName);
    const std::wstring ClassNameWStr = std::wstring(ClassNameStr.begin(), ClassNameStr.end());
    auto Class = reinterpret_cast<SDK::UClass*>(
        SDK::GUObjectArray.FindObject(ClassNameWStr));
    if (!Class) {
        UE_LOG(LogSDK, Warning, L"Failed to find class By Name: %s",
            ClassNameWStr);
        return nullptr;
    }
    ClassCache[ClassName] = Class;
    return Class;
}

void SDK::UObject::ProcessEvent(UFunction* Function, void* Parms) const {
    static void (*ProcessEvent)(const UObject * Object, UFunction * Func,
        void* Params) = decltype(ProcessEvent)(Offsets::UObject_ProcessEvent);
    return ProcessEvent(this, Function, Parms);
}

SDK::UObject* SDK::UClass::GetDefaultObj()
{
    return SDK::GUObjectArray.FindObject(std::wstring(L"Default__") + *this->GetFName().ToString());
}

SDK::UFunction* SDK::UFunction::FromName(std::wstring FunctionName)
{
    return reinterpret_cast<UFunction*>(SDK::GUObjectArray.FindObject(FunctionName));
}

bool SDK::UStruct::IsChildOf(const UStruct* SomeBase) const
{
    if (SomeBase == nullptr) {
        return false;
    }

    bool bOldResult = false;
    for (const UStruct* TempStruct = this; TempStruct;
        TempStruct = TempStruct->GetSuperStruct()) {
        if (TempStruct == SomeBase) {
            bOldResult = true;
            break;
        }
    }

    return bOldResult;
}