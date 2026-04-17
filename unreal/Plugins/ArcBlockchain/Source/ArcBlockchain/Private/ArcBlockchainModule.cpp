#include "ArcBlockchainModule.h"

#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ArcBlockchainSettings.h"

IMPLEMENT_MODULE(FArcBlockchainModule, ArcBlockchain)

void FArcBlockchainModule::StartupModule()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings(
            "Project",
            "Plugins",
            "ArcBlockchain",
            FText::FromString("ARC Blockchain"),
            FText::FromString("Configure ARC blockchain integration."),
            GetMutableDefault<UArcBlockchainSettings>()
        );
    }
}

void FArcBlockchainModule::ShutdownModule()
{
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "ArcBlockchain");
    }
}