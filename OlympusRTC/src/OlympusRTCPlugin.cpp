#include "OlympusRTCPlugin.h"

#include "ImagerPluginCore/PluginManager.h"

#include "RTC.h"

void InitPlugin() {
    // Imager is starting up. Create all objects and perform all work
    // needed to start operation.

    static std::unique_ptr<RTC> rtc = std::make_unique<RTC>();
    rtc->init();

    // the Manager object needs to know about all equipment provided by the plugin
    auto& manager = PluginManager::Manager();
    for (const auto& lightSource : rtc->getLightSources()) {
        manager.addLightSource(lightSource);
    }
    for (const auto& component : rtc->getContinuouslyMovableComponents()) {
        manager.addContinuouslyMovableComponent(component);
    }
}

void ShutdownPlugin() {
    // Imager is closing.
    // Perform any necessary cleanup here.
}
