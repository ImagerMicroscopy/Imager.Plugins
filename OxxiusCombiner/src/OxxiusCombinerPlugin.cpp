#include "OxxiusCombinerPlugin.h"

#include <format>
#include <stdexcept>

#include "ImagerPluginCore/ConfigManager.h"
#include "ImagerPluginCore/PluginManager.h"

#include "OxxiusCombiner.h"

const int MAX_OXXIUS_COMBINERS = 8;

static std::vector<std::shared_ptr<OxxiusCombiner>> sCombiners;

OxxiusCombiner::ModulationMode parseModulationMode(const std::string& modeStr) {
    if (modeStr == "NoModulation") return OxxiusCombiner::ModulationMode::NoModulation;
    if (modeStr == "DigitalModulation") return OxxiusCombiner::ModulationMode::DigitalModulation;
    if (modeStr == "AnalogModulation") return OxxiusCombiner::ModulationMode::AnalogModulation;
    throw std::runtime_error(std::format("Invalid ModulationMode '{}'. Allowed values: NoModulation, DigitalModulation, AnalogModulation", modeStr));
}

void InitPlugin() {
    ConfigManager& configManager = PluginManager::Manager().getConfigManager();
    
    for (int index = 1; index <= MAX_OXXIUS_COMBINERS; ++index) {
        std::string combinerKey = std::format("OxxiusCombiner_{}", index);
        auto portResponse = configManager.getStringSettingOrDefault(combinerKey / "Port", "");
        
        if (portResponse.value.empty()) {
            continue; // No combiner configured at this index
        }
        
        auto modeResponse = configManager.getStringSettingOrDefault(combinerKey / "ModulationMode", "NoModulation");
        OxxiusCombiner::ModulationMode modulationMode = parseModulationMode(modeResponse.value);
        
        auto turnOffResponse = configManager.getBoolSettingOrDefault(combinerKey / "TurnOffLCXOnStartupAndEnd", false);
        bool turnOffLCX = turnOffResponse.value;

        auto commResponse = configManager.getBoolSettingOrDefault(combinerKey / "PrintCommunication", false);
        bool printCommunication = commResponse.value;
        
        std::string name = std::format("Oxxius Combiner {}", index);
        
        try {
            auto combiner = std::make_shared<OxxiusCombiner>(name, portResponse.value, modulationMode, turnOffLCX);
            combiner->setPrintCommunication(true);
            combiner->initialize();
            PluginManager::Manager().addLightSource(combiner);
            sCombiners.push_back(combiner);
            PluginManager::Manager().Print(std::format("Initialized {} on port {}", name, portResponse.value));
        } catch (const std::exception& e) {
            PluginManager::Manager().Print(std::format("Failed to initialize {} on port {}: {}", name, portResponse.value, e.what()));
        }
    }
}

void ShutdownPlugin() {
    for (const auto& combiner : sCombiners) {
        combiner->shutdown();
    }
    sCombiners.clear();
}
