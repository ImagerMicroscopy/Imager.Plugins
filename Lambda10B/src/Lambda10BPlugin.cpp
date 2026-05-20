#include "Lambda10BPlugin.h"

#include <format>
#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "Lambda10B.h"

static std::vector<std::shared_ptr<Lambda10B>> sDevices;

void InitPlugin() {
    PluginManager& pluginManager = PluginManager::Manager();

    pluginManager.Print("Initializing Lambda10B plugin...");

    ConfigManager& configManager = pluginManager.getConfigManager();

    // allow up to 8 filterwheels
    for (int i = 1; i <= 8; ++i) {
        std::string fwName = std::format("Lambda10B_{}", i);
        auto portResponse = configManager.getStringSettingOrDefault(fwName / "Port", "");
        std::string portName = portResponse.value;

        auto baudrateResponse = configManager.getIntSettingOrDefault(fwName / "Baudrate", 115200);
        int baudrate = baudrateResponse.value;

        auto speedResponse = configManager.getIntSettingOrDefault(fwName / "Speed", 1);
        std::uint8_t fwSpeed = speedResponse.value;

        // up to 10 filters per device
        std::vector<std::string> filterNames;
        for (int f = 1; f <= 10; ++f) {
            std::string filterName = std::format("Filter{}", f);
            auto filterResponse = configManager.getStringSettingOrDefault(fwName / filterName, "");
            filterNames.push_back(filterResponse.value);
        }

        auto communicationResponse = configManager.getBoolSettingOrDefault(fwName / "PrintCommunication", false);
        bool printCommunication = communicationResponse.value;

        if (!portName.empty()) {
            auto lambda10b = std::make_shared<Lambda10B>(portName, baudrate, filterNames, fwSpeed);
            lambda10b->init();
            for (const auto& component : lambda10b->getDiscreteMovableComponents()) {
                pluginManager.addDiscreteMovableComponent(component);
            }
            sDevices.push_back(lambda10b);
        }
    }

}


void ShutdownPlugin() {
    for (auto &device : sDevices) {
        device->shutdown();
    }
}
