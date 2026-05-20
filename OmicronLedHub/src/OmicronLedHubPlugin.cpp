#include "OmicronLedHubPlugin.h"

#include <format>
#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "OmicronLedHub.h"

const int MAX_LEDHUBS = 8;

static std::vector<std::shared_ptr<OmicronLedHub>> sDevices;

void InitPlugin() {
    ConfigManager& configManager = PluginManager::Manager().getConfigManager();

    for (int index = 1; index <= MAX_LEDHUBS; ++index) {
        std::string ledHubKey = std::format("OmicronLedHub_{}", index);
        auto portResponse = configManager.getStringSettingOrDefault(ledHubKey / "Port", "");
        if (portResponse.value.empty()) {
            continue; // No ledHub configured at this index
        }
        std::string portName = portResponse.value;

        auto commResponse = configManager.getBoolSettingOrDefault(ledHubKey / "PrintCommunication", false);
        bool printCommunication = commResponse.value;
        
        std::string name = std::format("LedHub {}", index);
        
        try {
            auto ledHub = std::make_shared<OmicronLedHub>(name, portName);
            ledHub->setPrintCommunication(printCommunication);
            ledHub->initialize();
            PluginManager::Manager().addLightSource(ledHub);
            sDevices.push_back(ledHub);
            PluginManager::Manager().Print(std::format("Initialized {} on port {}", name, portName));
        } catch (const std::exception& e) {
            PluginManager::Manager().Print(std::format("Failed to initialize {} on port {}: {}", name, portName, e.what()));
        }
    }

}


void ShutdownPlugin() {
    for (auto &device : sDevices) {
        device->shutdown();
    }
}
