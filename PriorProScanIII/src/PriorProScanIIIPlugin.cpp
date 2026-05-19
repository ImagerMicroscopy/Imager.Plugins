#include "PriorProScanIIIPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "PriorProScanIII.h"

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    ConfigManager& configManager = PluginManager::Manager().getConfigManager();
    auto portResponse = configManager.getStringSettingOrDefault(std::string("PriorProScanIII") / "Port", "");
    std::string portName = portResponse.value;

    auto printCommsResponse = configManager.getBoolSettingOrDefault(std::string("PriorProScanIII") / "PrintCommunication", false);
    bool printCommunication = printCommsResponse.value;

    if (portName.empty()) {
        PluginManager::Manager().Print("No port configured for PriorProScanIII plugin, not initializing stage.");
        return;
    }

    auto stage = std::make_shared<PriorProScanIII>(portName, printCommunication);
    PluginManager::Manager().addMotorizedStage(stage);
}

void ShutdownPlugin() {
    // Imager is closing.
    // Perform any necessary cleanup here.
}
