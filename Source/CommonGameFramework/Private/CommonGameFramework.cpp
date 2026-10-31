#include "CommonGameFramework.h"

DEFINE_LOG_CATEGORY(LogCGF);

#define LOCTEXT_NAMESPACE "FCommonGameFrameworkModule"

void FCommonGameFrameworkModule::StartupModule()
{
}

void FCommonGameFrameworkModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCommonGameFrameworkModule, CommonGameFramework)
