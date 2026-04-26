#include "OlympusIX83Plugin.h"

#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "OlympusIX83.h"

void InitPlugin([[maybe_unused]] const std::filesystem::path& configDirPath) {
    static std::unique_ptr<OlympusIX83> microscope = std::make_unique<OlympusIX83>();

    PluginManager& pluginManager = PluginManager::Manager();
    for (const auto& lightSource : microscope->getLightSources()) {
        pluginManager.addLightSource(lightSource);
    }
    for (const auto& component : microscope->getDiscreteMovableComponents()) {
        pluginManager.addDiscreteMovableComponent(component);
    }
    for (const auto& component : microscope->getContinuouslyMovableComponents()) {
        pluginManager.addContinuouslyMovableComponent(component);
    }
    for (const auto& stage : microscope->getMotorizedStages()) {
        pluginManager.addMotorizedStage(stage);
    }
    for (const auto& robot : microscope->getRobots()) {
        pluginManager.addRobot(robot);
    }
}


void ShutdownPlugin() {

}

