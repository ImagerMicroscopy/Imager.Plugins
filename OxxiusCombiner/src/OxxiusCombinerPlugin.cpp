#include "OxxiusCombinerPlugin.h"

#include <format>
#include <stdexcept>

#include "ImagerPluginCore/ConfigManager.h"
#include "ImagerPluginCore/PluginManager.h"

#include "OxxiusCombiner.h"

const int MAX_OXXIUS_COMBINERS = 8;

OxxiusCombiner::ModulationMode parseModulationMode(const std::string& modeStr) {
    if (modeStr == "NoModulation") return OxxiusCombiner::ModulationMode::NoModulation;
    if (modeStr == "DigitalModulation") return OxxiusCombiner::ModulationMode::DigitalModulation;
    if (modeStr == "AnalogModulation") return OxxiusCombiner::ModulationMode::AnalogModulation;
    throw std::runtime_error(std::format("Invalid ModulationMode '{}'. Allowed values: NoModulation, DigitalModulation, AnalogModulation", modeStr));
}

bool parseBool(const std::string& value) {
    if (value == "True") return true;
    if (value == "False") return false;
    throw std::runtime_error(std::format("Invalid boolean value '{}'. Allowed values: True, False", value));
}

void InitPlugin() {
    ConfigManager& configManager = PluginManager::Manager().getConfigManager();
    
    for (int index = 1; index <= MAX_OXXIUS_COMBINERS; ++index) {
        std::string combinerKey = std::format("OxxiusCombiner_{}", index);
        ConfigPath portPath = ConfigPath(combinerKey) / "Port";
        auto portResponse = configManager.getSettingOrDefault(portPath, "");
        
        if (portResponse.first.empty()) {
            continue; // No combiner configured at this index
        }
        
        ConfigPath modePath = ConfigPath(combinerKey) / "ModulationMode";
        auto modeResponse = configManager.getSettingOrDefault(modePath, "NoModulation");
        OxxiusCombiner::ModulationMode modulationMode = parseModulationMode(modeResponse.first);
        
        ConfigPath turnOffPath = ConfigPath(combinerKey) / "TurnOffLCXOnStartupAndEnd";
        auto turnOffResponse = configManager.getSettingOrDefault(turnOffPath, "False");
        bool turnOffLCX = parseBool(turnOffResponse.first);

        auto commResponse = configManager.getSettingOrDefault(combinerKey / "PrintCommunication", "False");
        bool printCommunication = parseBool(commResponse.first);
        
        std::string name = std::format("Oxxius Combiner {}", index);
        
        try {
            auto combiner = std::make_shared<OxxiusCombiner>(name, portResponse.first, modulationMode, turnOffLCX);
            combiner->setPrintCommunication(printCommunication);
            PluginManager::Manager().addLightSource(combiner);
            PluginManager::Manager().Print(std::format("Initialized {} on port {}", name, portResponse.first));
        } catch (const std::exception& e) {
            PluginManager::Manager().Print(std::format("Failed to initialize {} on port {}: {}", name, portResponse.first, e.what()));
        }
    }
}

void ShutdownPlugin() {
}
