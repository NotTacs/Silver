#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include "SDK/AssertionMacros.h"
#include "SDK/GenericPlatformMath.h"
#include "SDK/LogVerbosity.h"
#include "SDK/LogCategory.h"
#include "SDK/LogMacros.h"
#include "SDK/Offsets.h"
#include "SDK/Memory.h"
#include "SDK/NumericLimits.h"
#include "SDK/UnrealContainers.h"
#include "SDK/external/memcury.h"
#include "SDK/MemoryLibrary.h"
#include "SDK/Basic.h"
#include "SDK/Version.h"
#include "SDK/PropertyFinder.h"
#include "SDK/DefineMacros.h"
#include "SDK/Engine_classes.h"

DEFINE_LOG_CATEGORY(LogSDK, Log)

namespace SDK
{
	/*Global Memory Library*/
	extern std::unique_ptr<MemoryLibrary> GMemLibrary;
	extern FUObjectArray GUObjectArray;
	extern FEngineVersion Engine_Version;
	extern FFortniteVersion Fortnite_Version;


	bool Init();
}