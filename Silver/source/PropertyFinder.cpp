#include "pch.h"
#include "../include/SDK.h"

SDK::PropertyInfo SDK::PropertyFinder::FindPropertyByName(const std::string& ClassName, const std::string& PropName) 
{
	PropertyInfo Info{};
	Info.PropName = PropName;
	Info.Offset = -1;

	std::wstring ClassNameWStr = std::wstring(ClassName.begin(), ClassName.end());
	UClass* StaticClass = static_cast<UClass*>(SDK::GUObjectArray.FindObject(ClassNameWStr));
	void* ret = nullptr;

	bool bSupportsFProperty = SDK::Engine_Version >= FEngineVersion(4, 25, 0);
	if (bSupportsFProperty) {
		FProperty* Prop = reinterpret_cast<FProperty*>(StaticClass->GetPropLink());
		if (!Prop) {

			return Info;
		}
		for (Prop; Prop = Prop->PropertyLinkNext;) {
			std::wstring NameWStr = *Prop->NamePrivate.ToString();
			std::string NameStr = std::string(NameWStr.begin(), NameWStr.end());
			if (NameStr == PropName) {
				ret = Prop;
				break;
			}
		}
	}
	else {
		UProperty* Prop = reinterpret_cast<UProperty*>(StaticClass->GetPropLink());
		if (!Prop) {
			return Info;
		}
		for (Prop; Prop = Prop->PropertyLinkNext;) {
			std::wstring NameWStr = *Prop->GetFName().ToString();
			std::string NameStr = std::string(NameWStr.begin(), NameWStr.end());
			if (NameStr == PropName) {
				ret = Prop;
				break;
			}
		}
	}

	const static bool bUseChildProperties =
		SDK::Offsets::Members::UStruct__ChildProperties != -1;
	if (bUseChildProperties) {
		for (FField* Next = StaticClass->GetChildrenProperties(); Next; Next = Next->Next) {
			FProperty* NextProp = reinterpret_cast<FProperty*>(Next);
			std::wstring NameWStr = *NextProp->NamePrivate.ToString();
			std::string NameStr = std::string(NameWStr.begin(), NameWStr.end());
			if (NameStr == PropName) {
				ret = NextProp;
				break;
			}
		}
	}
	else {
		for (UField* Next = StaticClass->GetChildren(); Next; Next = Next->Next) {
			if (!Next->GetClass() || *Next->GetClass()->GetFName().ToString() == L"Function" /*put Class check*/) continue;
			UProperty* NextProp =
				reinterpret_cast<UProperty*>(Next);
			std::wstring NameWStr = *NextProp->GetFName().ToString();
			std::string NameStr = std::string(NameWStr.begin(), NameWStr.end());
			if (NameStr == PropName) {
				ret = NextProp;
				break;
			}
		}
	}

	Info.Prop = ret;
	Info.Offset = ret ? bUseChildProperties || bSupportsFProperty ? ((FProperty*)ret)->Offset_Internal : ((UProperty*)ret)->Offset_Internal : 0;
	return Info;
}