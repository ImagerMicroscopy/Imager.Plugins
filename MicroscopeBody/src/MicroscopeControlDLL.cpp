#define COMPILING_IMAGERPLUGIN

#include "MicroscopeControlDLL.h"

#include <memory>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <string>

#include "Config.h"

#include "BaseMicroscopeClass.h"

#if defined (WITH_TI2)
	#include "NikonTi2.h"
#elif defined(WITH_DUMMYMICROSCOPE)
	#include "DummyMicroscope.h"
#elif defined(WITH_DONOTHINGMICROSCOPE)
	#include "DoNothingMicroscope.h"
#elif defined(WITH_TI1)
	#include "NikonTi1.h"
#elif defined(WITH_IX83)
	#include "OlympusIX83.h"
#endif

const char* ksDichroicName = "Dichroic";
const char* ksFilterWheelName = "Filterwheel";

std::shared_ptr<BaseMicroscopeClass> gTheMicroscope;
void(*printer)(const char*);

// A utility function to store a list of strings in buffers passed by Imager. Returns the number of items actually stored.
int StoreStringListInBuffers(const std::vector<std::string>& stringList, char** stringBuffers, int nBuffers, int maxNBytesPerName);

void EnsureHasMicroscope() {
    if (gTheMicroscope.get() != nullptr) {
		printer("Microscope already connected");
        return;
    }
	#if defined (WITH_TI2)
		printer("Connecting to Nikon Ti2 microscope");
        gTheMicroscope = std::shared_ptr<BaseMicroscopeClass>(new NikonTi2());
	#elif defined(WITH_DUMMYMICROSCOPE)
		printer("Connecting to dummy microscope");
		gTheMicroscope = std::shared_ptr<BaseMicroscopeClass>(new DummyMicroscope());
	#elif defined(WITH_DONOTHINGMICROSCOPE)
		printer("Connecting to do-nothing microscope");
		gTheMicroscope = std::shared_ptr<BaseMicroscopeClass>(new DoNothingMicroscope());
    #elif defined(WITH_TI1)
		printer("Connecting to Nikon Ti1 microscope");
        gTheMicroscope = std::shared_ptr<BaseMicroscopeClass>(new NikonTi1());
	#elif defined(WITH_IX83)
		printer("Connecting to Olympus IX83 microscope");
        gTheMicroscope = std::shared_ptr<BaseMicroscopeClass>(new OlympusIX83());
	#else
        #error "need to have a microscope make to connect to"
    #endif
}

int HandleExceptions(std::function<void()> func) {
    try {
        func();
    }
    catch (const std::exception& e) {
		printer(e.what());
        return -1;
    }
    catch (...) {
		printer("unknown exception in MicroscopeControl");
        return -2;
    }
    return 0;
}

int InitImagerPlugin(char* configurationDirPath, void(*printer)(const char*)) {
    return HandleExceptions([&] () {
		::printer = printer;

        EnsureHasMicroscope();
    });
}

void ShutdownImagerPlugin() {
    HandleExceptions([] () {
		printer("Disconnecting from microscope");
        gTheMicroscope.reset();
    });
}

int ImagerPluginAPIVersion(int* version)
{
	return HandleExceptions([&] () {
		*version = IMAGER_API_VERSION;
	});
}

int EquipmentName(char* name, int maxNBytesPerName)
{
	return HandleExceptions([&]() {
		std::string s = "Microscope Controller";
		if (s.size() >= maxNBytesPerName) throw std::runtime_error("Not enough bytes allocated for equipment name");
		strcpy(name, s.c_str());
	});
}

void ReturnStringItems(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned, const std::vector<std::string>& items) {
    if (maxNBytesPerName <= 0) {
		return;
    }
	*nNamesReturned = std::min((int)items.size(), nNames);
	for (int i = 0; i < *nNamesReturned; i += 1) {
		int nBytesToCopy = std::min(maxNBytesPerName - 1, (int)items.at(i).length()) + 1;	// +1 so we copy the null byte
		const char* dest = namesPtr[i];
		const char* src = items.at(i).c_str();
		memcpy(namesPtr[i], items.at(i).c_str(), nBytesToCopy);
	}
}

int ListAvailableLightSources(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    return HandleExceptions([&]() {
        std::vector<std::string> lss = gTheMicroscope->listAvailableLightSources();
        ReturnStringItems(namesPtr, nNames, maxNBytesPerName, nNamesReturned, lss);
    });
}

int ActivateLightSource(char* lightSourceName, char** channelNames, double* illuminationPowers, int nChannels) {
    return HandleExceptions([&]() {
		char* buffer = new char[1024];
		sprintf_s(buffer, 1024, "Activating lightSource: %s with channels: ", lightSourceName);
		printer(buffer);

		for (int i = 0; i < nChannels; i++) {
			sprintf_s(buffer, 1024, "Channel: %s with power: %f", channelNames[i], illuminationPowers[i]);
			printer(buffer);
		}

        if (nChannels < 0) {
            throw std::runtime_error("invalid number of channels");
        }
        std::vector<std::pair<std::string, double>> chs;
        for (int i = 0; i < nChannels; i += 1) {
            chs.push_back(std::pair<std::string, double>(channelNames[i], illuminationPowers[i]));
        }
        gTheMicroscope->activateLightSource(lightSourceName, chs);
    });
}
int DeactivateLightSource() {
    return HandleExceptions([&]() {
		printer("Deactivating lightSource");

        gTheMicroscope->deactivateLightSource();
    });
}

int ListAvailableChannels(char* lightSourceName, char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned, int* canControlPower, int* allowMultipleChannels) {
    return HandleExceptions([&]() {
        std::vector<std::string> chs = gTheMicroscope->listAvailableChannels(lightSourceName, canControlPower, allowMultipleChannels);
        
        ReturnStringItems(namesPtr, nNames, maxNBytesPerName, nNamesReturned, chs);
    });
}

int ListDiscreteMovableComponents(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
	return HandleExceptions([&] () {
        std::vector<std::string> fws;
		if (gTheMicroscope->hasMotorizedDichroic()) {
			fws.push_back(ksDichroicName);
		}
		if (gTheMicroscope->hasFilterWheel()) {
			fws.push_back(ksFilterWheelName);
		}
		ReturnStringItems(namesPtr, nNames, maxNBytesPerName, nNamesReturned, fws);
    });
}


int ListContinuouslyMovableComponents(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
	nNamesReturned = 0;	// no movable components are implemented for now
	return 0;
}

int ListDiscreteMovableComponentSettings(char* discreteComponentName, char** namesPtr, 
                                         int nNames, int maxNBytesPerName, int* nNamesReturned) {
	return HandleExceptions([&]() {
		std::vector<std::string> items;
		if (!strcmp(discreteComponentName, ksDichroicName)) {
			items = gTheMicroscope->listDichroicMirrors();
		} else if (!strcmp(discreteComponentName, ksFilterWheelName)) {
			items = gTheMicroscope->listAvailableFilters();
		} else {
			throw std::runtime_error("unknown filter wheel when listing filters");
		}
		ReturnStringItems(namesPtr, nNames, maxNBytesPerName, nNamesReturned, items);
	});
}

int ListContinuouslyMovableComponentRange(char* discreteComponentName, double* minValue, double* maxValue, double* increment) {
	// should not be called since we do not list any continous components for now
	return HandleExceptions([&]() {
		throw std::logic_error("ListContinuouslyMovableComponentRange() but no such components defined");
	});
}

int SetMovableComponents(int nDiscreteComponentNames, char** discreteComponentNames, char** discreteSettings,
                         int nContinuousComponentNames, char** continuousComponentNames, double* continuousSettings) {
	return HandleExceptions([&]() {
		if (nContinuousComponentNames != 0) {
			throw std::logic_error("SetMovableComponents() for continuous but no such components defined");
		}

		for (int i = 0; i < nDiscreteComponentNames; ++i) {
			std::string componentName(discreteComponentNames[i]);
			std::string filterName(discreteSettings[i]);
			std::string message;
			if (componentName == ksDichroicName) {
				gTheMicroscope->setDichroicMirror(filterName);
				message = "set dichroic to " + filterName;
			} else if (componentName == ksFilterWheelName) {
				gTheMicroscope->setFilter(filterName);
				message = "set filter to " + filterName;
			} else {
				throw std::logic_error("SetMovableComponents for " + componentName + "but not known");
			}
			printer(message.c_str());
		}
	});
}

int HasMotorizedStage(int* hasIt) {
    return HandleExceptions([&] () {
        *hasIt = gTheMicroscope->hasMotorizedStage();
    });
}

int MotorizedStageName(char* name, int maxNBytesPerName)
{
	return HandleExceptions([&] () {
		std::string s = "Stage";
		if (s.size() >= maxNBytesPerName) throw std::runtime_error("Not enough bytes allocated for stage name");
		strcpy(name, s.c_str());
	});
}

int SupportedStageAxes(int* x, int* y, int* z) {
    return HandleExceptions([&] () {
        auto axesVec = gTheMicroscope->supportedStageAxes();

        *x = std::find(axesVec.cbegin(), axesVec.cend(), xAxis) != axesVec.cend();
		*y = std::find(axesVec.cbegin(), axesVec.cend(), yAxis) != axesVec.cend();
		*z = std::find(axesVec.cbegin(), axesVec.cend(), zAxis) != axesVec.cend();
    });
}

int GetStagePosition(double* x, double* y, double* z, int* usingHardwareAF, int* afOffset) {
    return HandleExceptions([&] () {
        std::tie(*x, *y, *z, *usingHardwareAF, *afOffset) = gTheMicroscope->getStagePosition();
    });
}

int SetStagePosition(double x, double y, double z, int usingHardwareAF, int afOffset) {
    return HandleExceptions([&] () {
		char* buffer = new char[1024];
		sprintf_s(buffer, 1024, "Setting stage position to x:%f, y:%f, z:%f, hardwareAF:%i, afOffset:%i", x, y, z, usingHardwareAF, afOffset);
		printer(buffer);

        gTheMicroscope->setStagePosition(StagePosition(x, y, z, usingHardwareAF, afOffset));
    });
}

int ListRobots(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    return HandleExceptions([&] {
        std::vector<std::string> robotNames;
        *nNamesReturned = StoreStringListInBuffers(robotNames, namesPtr, nNames, maxNBytesPerName);
        if (*nNamesReturned != robotNames.size()) {
            throw std::runtime_error("Could not return all available robots");
        }
    });
}

int ListRobotPrograms(char* robotName, char** encodedProgramsInfoPtr) {
    return HandleExceptions([&] {
    });
}

void ReleaseRobotProgramsInfo(char* info) {
    delete[] info;
}

int ExecuteRobotProgram(char* robotName, char* encodedProgramCallParams) {
    return HandleExceptions([&]() {
    });
}

int RobotIsExecuting(char* robotName, int* isExecuting, char* possibleErrorMessage, int maxNBytesForErrorMessage) {
    return HandleExceptions([&]() {
    });
}

int StopRobots() {
    return HandleExceptions([&]() {
	});
}

int ListConnectedCameraNames(char **namesPtr, int nNames, int maxNBytesPerName, int *nNamesReturned) {
    return HandleExceptions([&]() {
        *nNamesReturned = 0;
    });
}

int GetCameraOptions(char* cameraName, char** encodedOptionsPtr) {
    return HandleExceptions([&]() {
        // The camera options need to be encoded as JSON. See CameraPropertiesEncoding.cpp/.h.
        // Essence of a possible implementation:
        // std::vector<CameraProperty> properties = camPtr->getCameraProperties();
        // std::vector<nlohmann::json> encodedProps;
        // for (const auto& p : properties) {
        //     encodedProps.push_back(p.encodeAsJSONObject());
        // }
        // nlohmann::json object;
        // object["properties"] = encodedProps;
        // std::string serialized = object.dump();
        // *encodedOptionsPtr = new char[serialized.size() + 1];
        // memcpy(*encodedOptionsPtr, serialized.data(), serialized.size() + 1);
    });
}

void ReleaseOptionsData(char* data) {
    // Release the data the plugin returned in GetCameraOptions(). This function will be called automatically when Imager
    // has received the data. Possible implementation:
    // delete[] data;
}

int SetCameraOption(char* cameraName, char* encodedOption) {
    return HandleExceptions([&]() {
        
    });
}

int GetFrameRate(char* cameraName, double* frameRate) {
    return -1;
}

int IsConfiguredForHardwareTriggering(char* cameraName, int* isConfiguredForHardwareTriggering) {
    return -1;
}

int AcquireSingleImage(char* cameraName, uint16_t** imagePtr, int* nRows, int* nCols) {
    return -1;
}

int StartAsyncAcquisition(char* cameraName) {
    return -1;
}

int StartBoundedAsyncAcquisition(char* cameraName, uint64_t nImagesToAcquire) {
    return -1;
}

int GetOldestImageAsyncAcquired(char* cameraName, uint32_t timeoutMillis, uint16_t** imagePtr, int* nRows, int* nCols, double* timeStamp) {
    return -1;
}

void ReleaseImageData(uint16_t* imagePtr) {
    
}

int AbortAsyncAcquisition(char* cameraName) {
    return -1;
}

void GetLastSCCamError(char* msgBuf, size_t bufSize) {
    msgBuf[0] = 0;
}

// A utility function to store a list of strings in buffers passed by Imager. Returns the number of items actually stored.
int StoreStringListInBuffers(const std::vector<std::string>& stringList, char** stringBuffers, int nBuffers, int maxNBytesPerName) {
    int nStrings = (int)stringList.size();
    int nItemsToStore = std::min(nStrings, nBuffers);
    for (int i = 0; i < nItemsToStore; ++i) {
        const std::string& item = stringList.at(i);
        if (item.size() > maxNBytesPerName - 1) {   // '-1' so the trailing nil can be stored
            throw std::runtime_error("buffer too small to store item \"" + item + "\"");
        }
        snprintf(stringBuffers[i], maxNBytesPerName, "%s", item.c_str());
    }
    return nItemsToStore;
}

