#include "NikonTiEPlugin.h"

#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "NikonTiE.h"

void InitPlugin([[maybe_unused]] const std::filesystem::path& configDirPath) {
    static std::unique_ptr<NikonTiE> microscope = std::make_unique<NikonTiE>();

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

