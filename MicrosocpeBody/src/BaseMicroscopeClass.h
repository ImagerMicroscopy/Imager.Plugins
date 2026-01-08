#ifndef BASEMICROSCOPECLASS_H
#define BASEMICROSCOPECLASS_H

#include <tuple>
#include <vector>
#include <string>

typedef std::tuple<double, double, double, bool, int> StagePosition;

enum StageAxis {
    xAxis,
    yAxis,
    zAxis
};

class BaseMicroscopeClass {
public:
    BaseMicroscopeClass() { ; }
    ~BaseMicroscopeClass() { ; }

    virtual std::vector<std::string> listAvailableLightSources() { return std::vector<std::string>(); }
    virtual std::vector<std::string> listAvailableChannels(const std::string& lsName, int* canControlPower, int* allowMultipleChannels) { return std::vector<std::string>(); }
    virtual void activateLightSource(const std::string& lsName, const std::vector<std::pair<std::string, double>>& channels) { ; }
    virtual void deactivateLightSource() { ; }

    virtual bool hasMotorizedDichroic() const { return false; }
    virtual std::vector<std::string> listDichroicMirrors() const { return std::vector<std::string>(); }
    virtual void setDichroicMirror(const std::string& dichroicMirrorName) { ; }

    virtual bool hasFilterWheel() const { return false; }
    virtual std::vector<std::string> listAvailableFilters() const { return std::vector<std::string>(); }
    virtual void setFilter(const std::string& filterName) { ; };

    virtual bool hasMotorizedStage() const { return false; }
    virtual std::vector<StageAxis> supportedStageAxes() const { return std::vector<StageAxis>(); }
    virtual StagePosition getStagePosition() {return StagePosition();}
    virtual void setStagePosition(const StagePosition& pos) { ; }
};

#endif
