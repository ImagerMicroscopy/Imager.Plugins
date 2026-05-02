#include "NikonTiE.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

#include "windows.h"

#include "Utils.h"

void HandleNikonException(std::function<void()> f) {
    try {
        f();
    }
    catch (_com_error err) {
#ifdef UNICODE
        throw std::runtime_error(ConvertWCSToMBS(err.ErrorMessage()));
#else
        throw std::runtime_error(err.ErrorMessage());
#endif
    }
}

NikonTiE::NikonTiE() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (_theMicroscope.CreateInstance(TISCOPELib::CLSID_NikonTi) != S_OK) {
        throw std::runtime_error("unable to find Nikon Ti1 microscope");
    }
	_switchEpiShutter(true);
}

NikonTiE::~NikonTiE() {
	_switchEpiShutter(false);
    _theMicroscope.Release();
    CoUninitialize();
}

std::vector<std::shared_ptr<DiscreteMovableComponent>> NikonTiE::getDiscreteMovableComponents() {
	std::vector<std::shared_ptr<DiscreteMovableComponent>> components;

	if (hasMotorizedDichroic()) {
		auto dichroicMirrorNames = listDichroicMirrors();
		components.push_back(std::make_shared<DiscreteMovableComponentFunctor>(
			"Dichroic Mirror",
			dichroicMirrorNames,
			[this](const std::string& name) { setDichroicMirror(name); }
		));
	}
	if (hasFilterWheel()) {
		auto filterNames = listAvailableFilters();
		components.push_back(std::make_shared<DiscreteMovableComponentFunctor>(
			"Filter",
			filterNames,
			[this](const std::string& name) { setFilter(name); }
		));
	}

	return components;
}

std::vector<std::shared_ptr<MotorizedStage>> NikonTiE::getMotorizedStages() {
	std::vector<std::shared_ptr<MotorizedStage>> stages;

	if (hasMotorizedStage()) {
		auto axes = supportedStageAxes();
		bool supportsX = (std::ranges::find(axes, xAxis) != axes.cend());
		bool supportsY = (std::ranges::find(axes, yAxis) != axes.cend());
		bool supportsZ = (std::ranges::find(axes, zAxis) != axes.cend());
		stages.push_back(std::make_shared<MotorizedStageFunctor>(
			"Stage",
			supportsX, supportsY, supportsZ,
			[this]() { return getStagePosition(); },
			[this](MotorizedStage::Position pos) { setStagePosition(pos); },
			[this]() { return false; },
            [this]() {}
		));
	}

	return stages;
}

bool NikonTiE::hasMotorizedDichroic() const {
    bool isMounted = false;
    HandleNikonException([&]() {
		TISCOPELib::IFilterBlockCassette *pFBC = nullptr;
		ScopedRunner sr([&]() {
			if (pFBC != nullptr) pFBC->Release();
		});
		if ((_theMicroscope->get_FilterBlockCassette1(&pFBC) == S_OK) && (long(pFBC->IsMounted) == TISCOPELib::StatusTrue)) {
			isMounted = true;
		}
    });
    return isMounted;
}

std::vector<std::string> NikonTiE::listDichroicMirrors() const {
    std::vector<std::string> dmList;
	HandleNikonException([&]() {
		auto pFBC = _theMicroscope->FilterBlockCassette1;
		if (long(pFBC->IsMounted) == TISCOPELib::StatusTrue) {
			auto pFilterBlocks = pFBC->FilterBlocks;
			for (int i = 1; i <= pFilterBlocks->GetCount(); i += 1) {
				auto filterBlockPtr = pFilterBlocks->Item[i];
				auto code = filterBlockPtr->GetCode();
				dmList.push_back(_getFilterBlockCodeString(code));
			}
		}
    });
	return dmList;
}

void NikonTiE::setDichroicMirror(const std::string& dichroicMirrorName) {
    std::vector<std::string> dmList = listDichroicMirrors();
    auto it = std::find(dmList.cbegin(), dmList.cend(), dichroicMirrorName);
    if (it == dmList.cend()) {
        throw std::runtime_error("unknown filter name");
    }
    int itemIndex = static_cast<int>(it - dmList.cbegin() + 1);   // index numbering starts from 1 in the Nikon SDK
    HandleNikonException([&]() {
		auto pFBC = _theMicroscope->FilterBlockCassette1;
		if (long(pFBC->IsMounted) == TISCOPELib::StatusTrue) {
			pFBC->Position = itemIndex;
		}
    });
}

bool NikonTiE::hasFilterWheel() const {
	return false;
}

std::vector<std::string> NikonTiE::listAvailableFilters() const {
	return std::vector<std::string>();
}
void NikonTiE::setFilter(const std::string& filterName) {

}

bool NikonTiE::hasMotorizedStage() const {
	auto availableAxes = supportedStageAxes();
    return (!availableAxes.empty());
}

std::vector<NikonTiE::StageAxis> NikonTiE::supportedStageAxes() const {
    std::vector<StageAxis> axes;
    HandleNikonException([&]() {
		TISCOPELib::IXDrive *pXDrive = nullptr;
		TISCOPELib::IYDrive *pYDrive = nullptr;
		TISCOPELib::IZDrive *pZDrive = nullptr;
		ScopedRunner sr([&]() {
			if (pXDrive != nullptr) pXDrive->Release();
			if (pYDrive != nullptr) pYDrive->Release();
			if (pZDrive != nullptr) pZDrive->Release();
		});
		if ((_theMicroscope->get_XDrive(&pXDrive) == S_OK) && (long(pXDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			axes.push_back(xAxis);
		}
		if ((_theMicroscope->get_YDrive(&pYDrive) == S_OK) && (long(pYDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			axes.push_back(yAxis);
		}
		if ((_theMicroscope->get_ZDrive(&pZDrive) == S_OK) && (long(pZDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			axes.push_back(zAxis);
		}
    });
    return axes;
}

NikonTiE::StagePosition NikonTiE::getStagePosition() {
    double x = -1.0, y = -1.0, z = -1.0;
    bool pfsIsActive = false;
    long pfsOffset = 0;

    HandleNikonException([&]() {
		TISCOPELib::IXDrive *pXDrive = nullptr;
		TISCOPELib::IYDrive *pYDrive = nullptr;
		TISCOPELib::IZDrive *pZDrive = nullptr;
		ScopedRunner sr([&]() {
			if (pXDrive != nullptr) pXDrive->Release();
			if (pYDrive != nullptr) pYDrive->Release();
			if (pZDrive != nullptr) pZDrive->Release();
		});
		if ((_theMicroscope->get_XDrive(&pXDrive) == S_OK) && (long(pXDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			long lPosition = pXDrive->Position;
			std::string unit = BStrToStdString(pXDrive->GetUnit());
			x = _toMicroMeter(lPosition, unit);
		}
		if ((_theMicroscope->get_YDrive(&pYDrive) == S_OK) && (long(pYDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			long lPosition = pYDrive->Position;
			std::string unit = BStrToStdString(pYDrive->GetUnit());
			y = _toMicroMeter(lPosition, unit);
		}
        if ((_theMicroscope->get_ZDrive(&pZDrive) == S_OK) && (long(pZDrive->IsMounted) == TISCOPELib::StatusTrue)) {
            long lPosition = pZDrive->Position;
			z = (double)lPosition / 40.0;	// "Z-drive stage position is leaner encoder in pulse unit" ???
        }
        if ((long)(_theMicroscope->PFS->IsMounted) == TISCOPELib::StatusTrue) {
            pfsIsActive = ((long)_theMicroscope->PFS->IsEnabled == TISCOPELib::StatusTrue);
            pfsOffset = _theMicroscope->PFS->Value;
        }
    });

    return StagePosition(x, y, z, pfsIsActive, pfsOffset);
}

void NikonTiE::setStagePosition(const StagePosition& pos) {
    HandleNikonException([&]() {
        double x, y, z;
        bool usingHardwareAF;
        int afOffset;
        std::tie(x, y, z, usingHardwareAF, afOffset) = pos;

		TISCOPELib::IXDrive *pXDrive = nullptr;
		TISCOPELib::IYDrive *pYDrive = nullptr;
		TISCOPELib::IZDrive *pZDrive = nullptr;
		ScopedRunner sr([&]() {
			if (pXDrive != nullptr) pXDrive->Release();
			if (pYDrive != nullptr) pYDrive->Release();
			if (pZDrive != nullptr) pZDrive->Release();
		});
		if ((_theMicroscope->get_XDrive(&pXDrive) == S_OK) && (long(pXDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			std::string unit = BStrToStdString(pXDrive->GetUnit());
			pXDrive->MoveAbsolute(static_cast<long>(_toUnit(x, unit)));
		}
		if ((_theMicroscope->get_YDrive(&pYDrive) == S_OK) && (long(pYDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			std::string unit = BStrToStdString(pYDrive->GetUnit());
			pYDrive->MoveAbsolute(static_cast<long>(_toUnit(y, unit)));
		}
		if ((_theMicroscope->get_ZDrive(&pZDrive) == S_OK) && (long(pZDrive->IsMounted) == TISCOPELib::StatusTrue)) {
			long zPos = static_cast<long>(z * 40.0);
			pZDrive->MoveAbsolute(zPos);
			//std::string unit = BStrToStdString(pZDrive->GetUnit());
			//pZDrive->MoveAbsolute(_toUnit(z, unit));
		}

        if ((long)_theMicroscope->PFS->IsMounted == TISCOPELib::StatusTrue) {
            if (usingHardwareAF) {
                _theMicroscope->PFS->Position = (long)afOffset;
                _theMicroscope->PFS->Enable();
            } else {
                _theMicroscope->PFS->Disable();
            }
        }
    });
}

void NikonTiE::_switchEpiShutter(bool openIt) {
	HandleNikonException([&]() {
		auto epiShutterPtr = _theMicroscope->EpiShutter;
		if (long(epiShutterPtr->IsMounted) == TISCOPELib::StatusTrue) {
			if (openIt) {
				epiShutterPtr->Open();
			}
			else {
				epiShutterPtr->Close();
			}
		}
	});
}

std::string NikonTiE::_getFilterBlockCodeString(int code) const {
	std::vector<std::string> codes = {
		"-----","ANALY","UV-2A","UV-2B","DAPI","UV-1A","V-2A","BV-2A","CFPHQ","BV-1A","B-3A","GFPHQ",
		"B-2A","B-1A","B-1E","FITC","GFP-L","GFP-B","YFPHQ","G-2A","G-2B","TRITC","G-1B","TxRed","Cy3",
		"Cy5","Cy7","L-385","L-455","L-470","L-505","L-525","L-590","L-625","DAP_1","FIT_1","TRI_1","TxR_1",
		"GFP_1","UV2_1","UV1_1","V2A_1","BV2_1","B2A_1","G2A_1","CFP_H","GFP_H","YFP_H","DAP_H","FIT_H","Cy5_H","mCh_H","Cy3_H"
	};
	return codes.at(code);
}

double NikonTiE::_toMicroMeter(double val, const std::string& unit) const {
	if (unit == "nm") {
		return val / 1000.0;
	}
	if (unit == "um") {
		return val;
	}
	throw std::runtime_error("unknown unit encountered");
}

double NikonTiE::_toUnit(double valmicrometer, const std::string& unit) const {
	if (unit == "nm") {
		return valmicrometer * 1000.0;
	}
	if (unit == "um") {
		return valmicrometer;
	}
	throw std::runtime_error("unknown unit encountered");
}
