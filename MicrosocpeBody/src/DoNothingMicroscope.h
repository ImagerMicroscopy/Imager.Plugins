#ifndef DONOTHINGMICROSCOPE_H
#define DONOTHINGMICROSCOPE_H

#include "BaseMicroscopeClass.h"

class DoNothingMicroscope : public BaseMicroscopeClass {
public:
	DoNothingMicroscope() { ; }
	~DoNothingMicroscope() { ; }

	bool hasMotorizedDichroic() const override { return false; }
	std::vector<std::string> listDichroicMirrors() const override { return std::vector<std::string>(); }
	void setDichroicMirror(const std::string& dichroicMirrorName) override { ; }

	bool hasFilterWheel() const override { return false; }
	std::vector<std::string> listAvailableFilters() const override { return std::vector<std::string>(); }
	void setFilter(const std::string& filterName) override { ; }

	bool hasMotorizedStage() const { return false; }
    std::vector<StageAxis> supportedStageAxes() const { return {}; }
	StagePosition getStagePosition() override { return StagePosition(); }
	void setStagePosition(const StagePosition& pos) override { ; }

private:
};

#endif
