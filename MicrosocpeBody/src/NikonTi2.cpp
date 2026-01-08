#include "Config.h"
#ifdef WITH_TI2

#include "NikonTi2.h"

#include "windows.h"

#include <stdexcept>

NikonTi2::NikonTi2() {
    lx_uint32 uiDeviceCount = 0;
    lx_int32* ppiDeviceTypeList = nullptr;
    lx_result err = MIC_GetDeviceList(uiDeviceCount, &ppiDeviceTypeList);
    if ((err != LX_OK) || (uiDeviceCount == 0)) {
        throw std::runtime_error("unable to find Nikon Ti2 microscope");
    }

    lx_uint64 uiConnectedAccessoryMask = 0;
    lx_uint32 uiErrMsgMaxSize = 255;
    lx_wchar  pwszErrMsg[256] = { 0 };
    err = MIC_Open(uiDeviceCount - 1, uiConnectedAccessoryMask, uiErrMsgMaxSize, pwszErrMsg);
    if (err != LX_OK) {
        throw std::runtime_error(_wStrToUtf8(pwszErrMsg).c_str());
    }
    _connectedAccessoryMask = uiConnectedAccessoryMask;

	_setFluorescenceShutter(true);
}

NikonTi2::~NikonTi2() {
	_setFluorescenceShutter(false);

    MIC_Close();
}

bool NikonTi2::hasMotorizedDichroic() const {
    return (_connectedAccessoryMask & MIC_ACCESSORY_MASK_TURRET1);
}

std::vector<std::string> NikonTi2::listDichroicMirrors() const {
    if (!hasMotorizedDichroic()) {
        return std::vector<std::string>();
    }

    std::vector<std::string> dichroicMirrors;
    MIC_MetaData metaData = _getMicroscopeMetaData();
    for (size_t i = 0; i < 8; i += 1) {
        if (metaData.sTurret1Filter[i].wsLongName != nullptr) {
            dichroicMirrors.push_back(_wStrToUtf8(metaData.sTurret1Filter[i].wsLongName));
        }
    }
    return dichroicMirrors;
}

void NikonTi2::setDichroicMirror(const std::string& dichroicMirrorName) {
    auto dmList = listDichroicMirrors();
    auto it = std::find(dmList.cbegin(), dmList.cend(), dichroicMirrorName);
    if (it == dmList.cend()) {
        throw std::runtime_error("dichroic mirror not found");
    }
    size_t dmIndex = it - dmList.cbegin() + 1;
    MIC_Data dataIn, dataOut;
    dataIn.uiDataUsageMask = MIC_DATA_MASK_TURRET1POS;
    dataIn.iTURRET1POS = dmIndex;
    lx_result err = MIC_DataSet(dataIn, dataOut, false);
    if (err != LX_OK) {
        throw std::runtime_error("error from MIC_DataSet() when changing dichroic");
    }
}

bool NikonTi2::hasFilterWheel() const {
    return (_connectedAccessoryMask & MIC_ACCESSORY_MASK_FILTERWHEEL_BARRIER1);
}

std::vector<std::string> NikonTi2::listAvailableFilters() const {
    if (!hasFilterWheel()) {
        return std::vector<std::string>();
    }

    std::vector<std::string> availableFilters;
    MIC_MetaData metaData = _getMicroscopeMetaData();
    for (size_t i = 0; i < 8; i += 1) {
        if (metaData.sBar1FilterWheel[i].wsLongName != nullptr) {
            availableFilters.push_back(_wStrToUtf8(metaData.sBar1FilterWheel[i].wsLongName));
        }
    }
    return availableFilters;
}

void NikonTi2::setFilter(const std::string& filterName) {
    auto filterList = listAvailableFilters();
    auto it = std::find(filterList.cbegin(), filterList.cend(), filterName);
    if (it == filterList.cend()) {
        throw std::runtime_error("dichroic mirror not found");
    }
    size_t filterIndex = it - filterList.cbegin() + 1;
    MIC_Data dataIn, dataOut;
    dataIn.uiDataUsageMask = MIC_DATA_MASK_FILTERWHEEL_BARRIER1;
    dataIn.iFILTERWHEEL_BARRIER1 = filterIndex;
    lx_result err = MIC_DataSet(dataIn, dataOut, false);
    if (err != LX_OK) {
        throw std::runtime_error("error from MIC_DataSet() when changing filter");
    }
}

bool NikonTi2::hasMotorizedStage() const {
    return ((_connectedAccessoryMask & MIC_ACCESSORY_MASK_ZSTAGE) && (_connectedAccessoryMask & MIC_ACCESSORY_MASK_XYSTAGE));
}

std::vector<StageAxis> NikonTi2::supportedStageAxes() const {
    std::vector<StageAxis> axes;
    if (_connectedAccessoryMask & MIC_DATA_MASK_XPOSITION) {
        axes.push_back(xAxis);
    }
    if (_connectedAccessoryMask & MIC_DATA_MASK_YPOSITION) {
        axes.push_back(yAxis);
    }
    if (_connectedAccessoryMask & MIC_DATA_MASK_ZPOSITION) {
        axes.push_back(zAxis);
    }
    return axes;
}

StagePosition NikonTi2::getStagePosition() {
    lx_result err;

    MIC_Data data = _getMicroscopeData();

    double x, y, z;
    err = MIC_Convert_Dev2Phys(MIC_DATA_MASK_XPOSITION, data.iXPOSITION, x);
    if (err)
        throw std::runtime_error("error calling MIC_Convert_Dev2Phys() for x");
    err = MIC_Convert_Dev2Phys(MIC_DATA_MASK_YPOSITION, data.iYPOSITION, y);
    if (err)
        throw std::runtime_error("error calling MIC_Convert_Dev2Phys() for y");
    err = MIC_Convert_Dev2Phys(MIC_DATA_MASK_ZPOSITION, data.iZPOSITION, z);
    if (err)
        throw std::runtime_error("error calling MIC_Convert_Dev2Phys() for z");
    bool pfsIsActive = data.iPFS_SWITCH;
    int pfsOffset = data.iPFS_OFFSET;

	y *= -1.0;	// compensate for different coordinate system between Prior and Nikon

    return StagePosition(x, y, z, pfsIsActive, pfsOffset);
}

void NikonTi2::setStagePosition(const StagePosition& pos) {
    lx_result err;

    lx_int32 xDevPos, yDevPos, zDevPos;
    double x, y, z;
    bool pfsIsActive;
    int pfsOffset;
    std::tie(x, y, z, pfsIsActive, pfsOffset) = pos;

	y *= -1.0;	// compensate for different coordinate system between Prior and Nikon

    err = MIC_Convert_Phys2Dev(MIC_DATA_MASK_XPOSITION, x, xDevPos);
    if (err != LX_OK)
        throw std::runtime_error("error calling MIC_Convert_Phys2Dev() for x");
    err = MIC_Convert_Phys2Dev(MIC_DATA_MASK_YPOSITION, y, yDevPos);
    if (err != LX_OK)
        throw std::runtime_error("error calling MIC_Convert_Phys2Dev() for y");
    err = MIC_Convert_Phys2Dev(MIC_DATA_MASK_ZPOSITION, z, zDevPos);
    if (err != LX_OK)
        throw std::runtime_error("error calling MIC_Convert_Phys2Dev() for z");

    MIC_Data data;
    MIC_Data dataOut;
    data.uiDataUsageMask = MIC_DATA_MASK_XPOSITION | MIC_DATA_MASK_YPOSITION | MIC_DATA_MASK_ZPOSITION | MIC_DATA_MASK_PFS_SWITCH;
    data.iXPOSITION = xDevPos;
    data.iYPOSITION = yDevPos;
    data.iZPOSITION = zDevPos;
    data.iPFS_SWITCH = pfsIsActive;
    if (pfsIsActive) {
        data.uiDataUsageMask |= MIC_DATA_MASK_PFS_OFFSET;
        data.iPFS_OFFSET = pfsOffset;
    }
    err = MIC_DataSet(data, dataOut, false);
    if (err != LX_OK) {
        throw std::runtime_error("error calling MIC_DataSet()");
    }
}

MIC_Data NikonTi2::_getMicroscopeData() const {
    lx_result err;
    MIC_Data data;
    data.uiDataUsageMask = MIC_DATA_MASK_FULL;
    err = MIC_DataGet(data);
    if (err != LX_OK) {
        throw std::runtime_error("unable get MIC_Data");
    }
    return data;
}

MIC_MetaData NikonTi2::_getMicroscopeMetaData() const {
    lx_result err;
    MIC_MetaData metaData;
    metaData.uiMetaDataUsageMask = MIC_METADATA_MASK_FULL;
    err = MIC_MetadataGet(metaData);
    if (err != LX_OK) {
        throw std::runtime_error("unable get MIC_MetaData");
    }
    return metaData;
}

void NikonTi2::_setFluorescenceShutter(bool open) {
	MIC_Data data;
	MIC_Data dataOut;
	data.uiDataUsageMask = MIC_DATA_MASK_TURRET1SHUTTER;
	data.iTURRET1SHUTTER = open;
	lx_result err = MIC_DataSet(data, dataOut, false);
	if (err != LX_OK) {
		throw std::runtime_error("error calling MIC_DataSet()");
	}
}

std::string NikonTi2::_wStrToUtf8(lx_wchar* wStr) const {
    char buf[512];
    int nBytesWritten = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(wStr), -1, buf, sizeof(buf) - 1, nullptr, nullptr);
    if (nBytesWritten == 0) {
        throw std::runtime_error("_wCharToUtf8() failed");
    }
    return std::string(buf);
}

#endif
