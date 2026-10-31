#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** The module's log category (CommonGameFramework rule: one per module, with the module prefix). */
DECLARE_LOG_CATEGORY_EXTERN(LogCGF, Log, All);

class FCommonGameFrameworkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
