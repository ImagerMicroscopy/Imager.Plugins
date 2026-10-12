#include "ThorlabsFW102CPlugin.h"

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "ImagerPluginCore/PluginManager.h"

#include "ThorlabsFW102C.h"

const int MAX_FILTER_WHEELS = 8;
const int MAX_FILTERS = 12;  // the FW102C has 6 or 12 positions, numbered from 1

static std::vector<std::shared_ptr<ThorlabsFW102C>> sDevices;

void InitPlugin() {
    PluginManager& pluginManager = PluginManager::Manager();

    pluginManager.Print("Initializing ThorlabsFW102C plugin...");

    ConfigManager& configManager = pluginManager.getConfigManager();

    // allow up to 8 filter wheels
    for (int index = 1; index <= MAX_FILTER_WHEELS; ++index) {
        std::string fwKey = std::format("ThorlabsFW102C_{}", index);
        auto portResponse = configManager.getStringSettingOrDefault(fwKey / "Port", "");
        std::string portName = portResponse.value;

        auto baudrateResponse = configManager.getIntSettingOrDefault(fwKey / "Baudrate", 115200);
        int baudrate = baudrateResponse.value;

        auto commResponse = configManager.getBoolSettingOrDefault(fwKey / "PrintCommunication", false);
        bool printCommunication = commResponse.value;

        // filter names by position (element i is position i+1); empty means not configured
        std::vector<std::string> filterNames(MAX_FILTERS);
        for (int f = 1; f <= MAX_FILTERS; ++f) {
            auto filterResponse = configManager.getStringSettingOrDefault(fwKey / std::format("Filter{}", f), "");
            filterNames[f - 1] = filterResponse.value;
        }

        if (portName.empty()) {
            continue;  // no filter wheel configured at this index
        }

        try {
            auto wheel = std::make_shared<ThorlabsFW102C>(index, portName, baudrate, filterNames);
            wheel->setPrintCommunication(printCommunication);
            wheel->init();
            for (const auto& component : wheel->getDiscreteMovableComponents()) {
                pluginManager.addDiscreteMovableComponent(component);
            }
            sDevices.push_back(wheel);
            pluginManager.Print(std::format("Initialized ThorlabsFW102C_{} on port {}", index, portName));
        } catch (const std::exception& e) {
            pluginManager.Print(std::format("Failed to initialize ThorlabsFW102C_{} on port {}: {}",
                                             index, portName, e.what()));
        }
    }
}

void ShutdownPlugin() {
    for (auto& device : sDevices) {
        device->shutdown();
    }
    sDevices.clear();
}
