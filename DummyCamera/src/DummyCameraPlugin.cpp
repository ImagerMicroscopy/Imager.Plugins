#include "DummyCameraPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "DummyCamera.h"

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    auto& manager = PluginManager::Manager();

    auto cam1 = std::make_shared<SimpleCamera>();
    auto cam2 = std::make_shared<SimpleCamera>();

    manager.addCamera(cam1);
    manager.addCamera(cam2);
}

void ShutdownPlugin() {
    // Imager is closing.
    // Perform any necessary cleanup here.
}
