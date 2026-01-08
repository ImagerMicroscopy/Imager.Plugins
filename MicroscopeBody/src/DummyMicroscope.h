#ifndef DUMMYMICROSCOPE_H
#define DUMMYMICROSCOPE_H

#include "BaseMicroscopeClass.h"

#define kDM1 ("DM1")
#define kDM2 ("DM2")
#define kFilter1 ("F1")
#define kFilter2 ("F2")

class DummyMicroscope : public BaseMicroscopeClass {
public:
	DummyMicroscope() :
		_currentDM(kDM1)
	  , _currentFilter(kFilter1)
	{}
	~DummyMicroscope() { ; }

	std::vector<std::string> listAvailableLightSources();
	std::vector<std::string> listAvailableChannels(const std::string& lsName, int* canControlPower, int* allowMultipleChannels);
	void activateLightSource(const std::string& lsName, const std::vector<std::pair<std::string, double>>& channels) {}
	void deactivateLightSource() {};

	bool hasMotorizedDichroic() const override { return true; }
	std::vector<std::string> listDichroicMirrors() const override { return std::vector<std::string>({ kDM1, kDM2 }); }
	void setDichroicMirror(const std::string& dichroicMirrorName) override;

	bool hasFilterWheel() const override { return true; }
	std::vector<std::string> listAvailableFilters() const override { return std::vector<std::string>({ kFilter1, kFilter2 }); }
	void setFilter(const std::string& filterName) override;

	bool hasMotorizedStage() const { return true; }
    std::vector<StageAxis> supportedStageAxes() const override { return { xAxis, yAxis, zAxis }; }
    StagePosition getStagePosition() override { return _stagePosition; }
	void setStagePosition(const StagePosition& pos) override { _stagePosition = pos; }

private:
	std::string _currentDM;
	std::string _currentFilter;
	std::tuple<double, double, double, bool, int> _stagePosition;
};

#endif
