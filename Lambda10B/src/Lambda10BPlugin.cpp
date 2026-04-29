#include "Lambda10BPlugin.h"

#include <format>
#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "Lambda10B.h"

void InitPlugin() {
    PluginManager& pluginManager = PluginManager::Manager();

    pluginManager.Print("Initializing Lambda10B plugin...");

    ConfigManager& configManager = pluginManager.getConfigManager();

    std::vector<std::shared_ptr<Lambda10B>> devices;
    // allow up to 8 filterwheels
    for (int i = 1; i <= 8; ++i) {
        std::string fwName = std::format("Lambda10B_{}", i);
        auto portResponse = configManager.getSettingOrDefault(fwName / "Port", "");
        std::string portName = portResponse.first;

        auto baudrateResponse = configManager.getSettingOrDefault(fwName / "Baudrate", "115200");
        int baudrate = std::stoi(baudrateResponse.first);

        auto speedResponse = configManager.getSettingOrDefault(fwName / "Speed", "1");
        std::uint8_t fwSpeed = std::stoi(speedResponse.first);

        // up to 10 filters per device
        std::vector<std::string> filterNames;
        for (int f = 1; f <= 10; ++f) {
            std::string filterName = std::format("Filter{}", f);
            auto filterResponse = configManager.getSettingOrDefault(fwName / filterName, "");
            filterNames.push_back(filterResponse.first);
        }

        if (!portName.empty()) {
            auto lambda10b = std::make_shared<Lambda10B>(portName, baudrate, filterNames, fwSpeed);
            lambda10b->init();
            for (const auto& component : lambda10b->getDiscreteMovableComponents()) {
                pluginManager.addDiscreteMovableComponent(component);
            }
            devices.push_back(lambda10b);
        }
    }

}


void ShutdownPlugin() {

}

