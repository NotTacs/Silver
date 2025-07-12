#pragma once

#define SET_OFFSET(RetType, MemberName, Offset) \
RetType MemberName() const { \
return *reinterpret_cast<RetType*>(reinterpret_cast<uintptr_t>(this) + Offset); \
}

#define DEFINE_MEMBER(RetType, ClassName, PropName) \
inline RetType Get##PropName() { \
    static int PropOffset = -1; \
    if (PropOffset == -1) { \
        PropOffset = SDK::PropertyFinder::FindPropertyByName((#ClassName + 1), #PropName).Offset; \
    } \
    return *reinterpret_cast<std::remove_reference_t<RetType>*>(uintptr_t(this) + PropOffset); \
}

#define GENERATED_CLASS_BODY() \
public: \
static SDK::UClass* StaticClass();

#define GENERATED_STRUCT_BODY() \
public: \
static SDK::UStruct* StaticStruct();

#define GENERATE_CLASS_BODY(ClassName) \
SDK::UClass* SDK::##ClassName::StaticClass() { \
return SDK::StaticClassImpl((#ClassName + 1)); \
} \

#define GENERATE_STRUCT_BODY(StructName) \
SDK::UStruct* SDK::##StructName::StaticStruct() { \
return SDK::StaticClassImpl((#StructName + 1));\
} \