#ifndef NIKONTIE_H
#define NIKONTIE_H

#include <vector>
#include <string>
#include <cstdint>

#include "ImagerPluginCore/DeviceTemplates.h"

#import "../lib/x64/NikonTi.dll" named_guids

class NikonTiE {
public:
    enum StageAxis {
        xAxis,
        yAxis,
        zAxis
    };

    using StagePosition = MotorizedStage::Position;

    NikonTiE();
    ~NikonTiE();

    NikonTiE(const NikonTiE&) = delete;
    NikonTiE& operator=(const NikonTiE&) = delete;

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
    std::vector<StageAxis> supportedStageAxes() const;
    StagePosition getStagePosition();
    void setStagePosition(const StagePosition& pos);

private:
	void _switchEpiShutter(bool openIt);

	std::string _getFilterBlockCodeString(int code) const;
	double _toMicroMeter(double val, const std::string& unit) const;
	double _toUnit(double valmicrometer, const std::string& unit) const;

    TISCOPELib::INikonTiPtr _theMicroscope;
};

#endif