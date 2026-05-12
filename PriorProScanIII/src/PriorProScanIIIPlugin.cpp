#include "PriorProScanIIIPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "PriorProScanIII.h"

bool parseBool(const std::string& value) {
    if (value == "True") return true;
    if (value == "False") return false;
    throw std::runtime_error(std::format("Invalid boolean value '{}'. Allowed values: True, False", value));
}

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    ConfigManager& configManager = PluginManager::Manager().getConfigManager();
    auto portResponse = configManager.getSettingOrDefault(std::string("PriorProScanIII") / "Port", "");
    std::string portName = portResponse.first;

    auto printCommsResponse = configManager.getSettingOrDefault(std::string("PriorProScanIII") / "PrintCommunication", "False");
    bool printCommunication = parseBool(printCommsResponse.first);

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
