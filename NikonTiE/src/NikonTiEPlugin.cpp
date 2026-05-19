#include "NikonTiEPlugin.h"

#include <memory>

#include "ImagerPluginCore/PluginManager.h"

#include "NikonTiE.h"

static std::unique_ptr<NikonTiE> sMicroscope;

void InitPlugin() {
    sMicroscope = std::make_unique<NikonTiE>();

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

