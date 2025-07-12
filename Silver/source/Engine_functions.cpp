#include "pch.h"
#include "../include/SDK.h"

SDK::UEngine* SDK::UEngine::GetEngine() {
    static UEngine* GEngine = nullptr;
    for (int i = 0; i < SDK::GUObjectArray.GetObjectArrayNum(); i++) {
        SDK::FUObjectItem* IndexedObject =
            SDK::GUObjectArray.IndexToObject(i);
        if (!IndexedObject)
            continue;
        UObject* Object = static_cast<UObject*>(IndexedObject->Object);
        if (!Object)
            continue;
        std::wstring ObjectName = *Object->GetFName().ToString();
        if (ObjectName.contains(L"FortEngine_")) {
            GEngine = (UEngine*)Object;
            break;
        }
    }

    return GEngine;
}