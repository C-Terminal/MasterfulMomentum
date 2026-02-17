#include "MMCoreModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMCore, Log, All);

#define LOCTEXT_NAMESPACE "FMMCoreModule"

void FMMCoreModule::StartupModule()
{
	UE_LOG(LogMMCore, Log, TEXT("MMCore module starting up"));
}

void FMMCoreModule::ShutdownModule()
{
	UE_LOG(LogMMCore, Log, TEXT("MMCore module shutting down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMMCoreModule, MMCore)