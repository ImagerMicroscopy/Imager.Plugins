#include "DummyEquipmentPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "DummyEquipment.h"

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    // the Manager object needs to know about all equipment provided by the plugin
    auto& manager = PluginManager::Manager();
    manager.addLightSource(std::make_shared<DummyLightSource>());
    manager.addDiscreteMovableComponent(std::make_shared<DummyFilterWheel>());
    manager.addContinuouslyMovableComponent(std::make_shared<DummyPolarizer>());
    manager.addMotorizedStage(std::make_shared<DummyMotorizedStage>());
    manager.addRobot(std::make_shared<DummyRobot>());
}

void ShutdownPlugin() {
    // Imager is closing.
    // Perform any necessary cleanup here.
}
