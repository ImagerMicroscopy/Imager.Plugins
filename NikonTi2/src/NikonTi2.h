#ifndef NIKONTI2_H
#define NIKONTI2_H

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <tuple>

#include "ImagerPluginCore/DeviceTemplates.h"

#include "NikonTi2/new_mic_sdk2.h"
#include "NikonTi2/new_mic_sdk2_DedicatedCommand.h"

class NikonTi2 {
public:
    NikonTi2();
    ~NikonTi2();

    NikonTi2(const NikonTi2&) = delete;
    NikonTi2& operator=(const NikonTi2&) = delete;

    std::vector<std::shared_ptr<LightSource>> getLightSources() { return {}; }
    std::vector<std::shared_ptr<DiscreteMovableComponent>> getDiscreteMovableComponents();
    std::vector<std::shared_ptr<ContinuouslyMovableComponent>> getContinuouslyMovableComponents() { return {}; }
    std::vector<std::shared_ptr<MotorizedStage>> getMotorizedStages();
    std::vector<std::shared_ptr<Robot>> getRobots() { return {}; }

private:

    bool hasMotorizedDichroic() const;
    std::vector<std::string> listDichroicMirrors() const;
    void setDichroicMirror(const std::string& dichroicMirrorName);

    bool hasFilterWheel() const;
    std::vector<std::string> listAvailableFilters() const;
    void setFilter(const std::string& filterName);

    bool hasMotorizedStage() const;
    MotorizedStage::Position getStagePosition();
    void setStagePosition(const MotorizedStage::Position& pos);

private:
    MIC_Data _getMicroscopeData() const;
    MIC_MetaData _getMicroscopeMetaData() const;
	void _setFluorescenceShutter(bool open);
    std::string _wStrToUtf8(lx_wchar* wStr) const;

    std::uint64_t _connectedAccessoryMask;
};

#endif