#pragma once

#include "Modules/ModuleManager.h"

class FCommonGameFrameworkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
