#include "pch.h"
#include "../include/SDK.h"


void SDK::FLogCategoryBase::SetVerbosity(ELogVerbosity::Type NewVerbosity) 
{
	const ELogVerbosity::Type OldVerbosity = Verbosity;
	Verbosity = FMath::Min<ELogVerbosity::Type>(CompileTimeVerbosity, (ELogVerbosity::Type)(NewVerbosity & ELogVerbosity::VerbosityMask));
	DebugBreakOnLog = !!(NewVerbosity & ELogVerbosity::BreakOnLog);
	checkSlow(!(Verbosity & ELogVerbosity::BreakOnLog));
}