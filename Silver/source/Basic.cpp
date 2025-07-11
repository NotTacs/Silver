#include "pch.h"
#include "../include/SDK.h"

SDK::FString SDK::FName::ToString() const {
    static FString& (*InternalToString)(const FName*, FString&) =
        decltype(InternalToString)(Offsets::FName_ToString);

    FString Ret = InternalToString(this, Ret);
    return Ret;
}