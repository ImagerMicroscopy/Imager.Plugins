#include "NikonTi2Plugin.h"

#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "NikonTi2.h"

static std::unique_ptr<NikonTi2> sMicroscope;

void InitPlugin() {
    sMicroscope = std::make_unique<NikonTi2>();

    PluginManager& pluginManager = PluginManager::Manager();
    for (const auto& lightSource : sMicroscope->getLightSources()) {
        pluginManager.addLightSource(lightSource);
    }
    for (const auto& component : sMicroscope->getDiscreteMovableComponents()) {
        pluginManager.addDiscreteMovableComponent(component);
    }
    for (const auto& component : sMicroscope->getContinuouslyMovableComponents()) {
        pluginManager.addContinuouslyMovableComponent(component);
    }
    for (const auto& stage : sMicroscope->getMotorizedStages()) {
        pluginManager.addMotorizedStage(stage);
    }
    for (const auto& robot : sMicroscope->getRobots()) {
        pluginManager.addRobot(robot);
    }
}


void ShutdownPlugin() {
    if (sMicroscope) {
        sMicroscope->shutdown();
    }
}

