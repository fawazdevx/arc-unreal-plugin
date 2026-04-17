#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Simple module class for the ArcBlockchain plugin.
 */
class FArcBlockchainModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};