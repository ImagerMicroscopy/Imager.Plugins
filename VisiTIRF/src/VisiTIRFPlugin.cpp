#include "VisiTIRFPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "VisiTIRFConnectionHandler.h"

static std::unique_ptr<VisiTIRFConnectionHandler> gVisiTIRF;

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    gVisiTIRF = std::make_unique<VisiTIRFConnectionHandler>();
    gVisiTIRF->startConnection();

    // the Manager object needs to know about all equipment provided by the plugin
    auto& manager = PluginManager::Manager();
    for (const auto& component : gVisiTIRF->getContinuouslyMovableComponents()) {
        manager.addContinuouslyMovableComponent(component);
    }
}

void ShutdownPlugin() {
    // Imager is closing.
    // Perform any necessary cleanup here.

    gVisiTIRF->closeConnection();
}
