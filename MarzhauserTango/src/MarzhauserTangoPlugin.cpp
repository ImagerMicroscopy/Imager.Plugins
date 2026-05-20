#include "MarzhauserTangoPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "MarzhauserTango.h"

static std::shared_ptr<MarzhauserTango> sStage;

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    ConfigManager& configManager = PluginManager::Manager().getConfigManager();
    auto portResponse = configManager.getStringSettingOrDefault(std::string("MarzhauserTango") + "/" + std::string("Port"), "");
    std::string portName = portResponse.value;

    if (portName.empty()) {
        PluginManager::Manager().Print("No port configured for MarzhauserTango plugin, not initializing stage.");
        return;
    }

    sStage = std::make_shared<MarzhauserTango>(portName, 57600, 30000);
    PluginManager::Manager().addMotorizedStage(sStage);
}

void ShutdownPlugin() {
    if (sStage) {
        sStage->shutdown();
    }
}
