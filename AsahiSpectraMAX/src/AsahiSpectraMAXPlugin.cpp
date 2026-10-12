#include "AsahiSpectraMAXPlugin.h"

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ImagerPluginCore/PluginManager.h"

#include "AsahiSpectraMAX.h"

const int MAX_LAMPS = 8;
const int MAX_FILTERS = 8;  // the device has 8 filter positions, numbered from 1

static std::vector<std::shared_ptr<AsahiSpectraMAX>> sDevices;

void InitPlugin() {
    PluginManager& pluginManager = PluginManager::Manager();

    pluginManager.Print("Initializing AsahiSpectraMAX plugin...");

    ConfigManager& configManager = pluginManager.getConfigManager();

    // allow up to 8 lamps
    for (int index = 1; index <= MAX_LAMPS; ++index) {
        std::string lampKey = std::format("AsahiSpectraMAX_{}", index);
        auto portResponse = configManager.getStringSettingOrDefault(lampKey / "Port", "");
        std::string portName = portResponse.value;

        auto baudrateResponse = configManager.getIntSettingOrDefault(lampKey / "Baudrate", 9600);
        int baudrate = baudrateResponse.value;

        auto commResponse = configManager.getBoolSettingOrDefault(lampKey / "PrintCommunication", false);
        bool printCommunication = commResponse.value;

        // filter names by position, numbered from 1; the number is the filter index on the device
        std::vector<std::pair<std::string, int>> channels;
        for (int f = 1; f <= MAX_FILTERS; ++f) {
            auto filterResponse = configManager.getStringSettingOrDefault(lampKey / std::format("Filter{}", f), "");
            if (!filterResponse.value.empty()) {
                channels.emplace_back(std::format("F{} - {}", f, filterResponse.value), f);
            }
        }

        if (portName.empty()) {
            continue;  // no lamp configured at this index
        }

        std::string name = std::format("AsahiSpectraMAX {}", index);

        try {
            auto lamp = std::make_shared<AsahiSpectraMAX>(name, portName, baudrate, channels);
            lamp->setPrintCommunication(printCommunication);
            lamp->initialize();
            pluginManager.addLightSource(lamp);
            sDevices.push_back(lamp);
            pluginManager.Print(std::format("Initialized {} on port {}", name, portName));
        } catch (const std::exception& e) {
            pluginManager.Print(std::format("Failed to initialize {} on port {}: {}", name, portName, e.what()));
        }
    }
}

void ShutdownPlugin() {
    for (auto& device : sDevices) {
        device->shutdown();
    }
    sDevices.clear();
}
