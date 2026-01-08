#ifndef NIKONTI2_H
#define NIKONTI2_H

#include "BaseMicroscopeClass.h"

#include <vector>
#include <string>
#include <cstdint>

#include "NikonTi2/new_mic_sdk2.h"
#include "NikonTi2/new_mic_sdk2_DedicatedCommand.h"

class NikonTi2 : public BaseMicroscopeClass {
public:
    NikonTi2();
    ~NikonTi2();

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
    MIC_Data _getMicroscopeData() const;
    MIC_MetaData _getMicroscopeMetaData() const;
	void _setFluorescenceShutter(bool open);
    std::string _wStrToUtf8(lx_wchar* wStr) const;

    std::uint64_t _connectedAccessoryMask;
};

#endif