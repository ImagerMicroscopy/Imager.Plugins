#ifndef NIKONTI_H
#define NIKONTI_H

#include "BaseMicroscopeClass.h"

#include <vector>
#include <string>
#include <cstdint>

#import "C:\Users\peter\Documents\C++\microscopecontrol\lib\x64\NikonTi.dll" named_guids

class NikonTi1 : public BaseMicroscopeClass {
public:
    NikonTi1();
    ~NikonTi1();

    bool hasMotorizedDichroic() const override;
    std::vector<std::string> listDichroicMirrors() const override;
    void setDichroicMirror(const std::string& dichroicMirrorName) override;

	bool hasFilterWheel() const override;
	std::vector<std::string> listAvailableFilters() const override;
	void setFilter(const std::string& filterName) override;

    bool hasMotorizedStage() const;
    std::vector<StageAxis> supportedStageAxes() const override;
    StagePosition getStagePosition() override;
    void setStagePosition(const StagePosition& pos) override;

private:
	void _switchEpiShutter(bool openIt);

	std::string _getFilterBlockCodeString(int code) const;
	double _toMicroMeter(double val, const std::string& unit) const;
	double _toUnit(double valmicrometer, const std::string& unit) const;

    TISCOPELib::INikonTiPtr _theMicroscope;
};

#endif