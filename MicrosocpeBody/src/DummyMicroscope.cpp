#include "Config.h"
#ifdef WITH_DUMMYMICROSCOPE

#include <stdexcept>

#include "DummyMicroscope.h"

std::vector<std::string> DummyMicroscope::listAvailableLightSources()
{
	return { "LightSource 1", "LightSource 2" };
}

std::vector<std::string> DummyMicroscope::listAvailableChannels(const std::string & lsName, int* canControlPower, int* allowMultipleChannels)
{
	if (lsName == "LightSource 1") {
		*canControlPower = false;
		*allowMultipleChannels = false;
		return { "Channel 1.1", "Channel 1.2" };
	}
	else if (lsName == "LightSource 2") {
		*canControlPower = true;
		*allowMultipleChannels = true;
		return { "Channel 2.1", "Channel 2.2" };
	}
	else {
		throw std::runtime_error("unknown lightSource name");
	}
}

void DummyMicroscope::setDichroicMirror(const std::string& dichroicMirrorName) {
	if ((dichroicMirrorName != kDM1) && (dichroicMirrorName != kDM2)) {
		throw std::runtime_error("unknown dichroic name");
	}
	_currentDM = dichroicMirrorName;
}

void DummyMicroscope::setFilter(const std::string& filterName) {
	if ((filterName != kFilter1) && (filterName != kFilter2)) {
		throw std::runtime_error("unknown dichroic name");
	}
	_currentFilter = filterName;
}

#endif