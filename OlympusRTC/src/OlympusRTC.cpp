#define COMPILING_IMAGERPLUGIN
#include "OlympusRTC.h"

#include <cstring>
#include <functional>
#include <stdexcept>

#include "RTC.h"

std::function<void(const char*)> gPrinter;
RTC gRTC;

int HandleExceptions(const std::function<void()>& func) {
    try {
        func();
    }
    catch (const std::exception& e) {
		gPrinter(e.what());
        return -1;
    }
    catch (...) {
		gPrinter("unknown exception in OlympusRTC");
        return -2;
    }
    return 0;
}

// A utility function to store a list of strings in buffers passed by Imager. Returns the number of items actually stored.
int StoreStringListInBuffers(const std::vector<std::string>& stringList, char** stringBuffers, int nBuffers, int maxNBytesPerName);


int InitImagerPlugin(char* configurationDirPath, void(*printer)(const char*)) {
    return HandleExceptions([&]() {
        gPrinter = printer;
        gRTC.init();
        printf("Successfully initialized\n");
    });
}

void ShutdownImagerPlugin() {;}

int ImagerPluginAPIVersion(int* version) {
    *version = IMAGER_API_VERSION;
    return 0;
}

int EquipmentName(char* name, int maxNBytesPerName) {
    strcpy(name, "OlympusRTC");
    return 0;
}

int ListAvailableLightSources(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    strcpy(namesPtr[0], "RTC");
    *nNamesReturned = 1;
    return 0;
}

int ListAvailableChannels(char* lightSourceName, char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned,
                          int* canControlPower, int* allowMultipleChannels) {
    return HandleExceptions([=]() {
        if (strcmp(lightSourceName, "RTC") != 0) {
            throw std::runtime_error("RTC only supports RTC lightsource");
        }
        std::vector<std::string> laserIdentifiers = gRTC.listAvailableLasers();
        int nNamesToReturn = std::min(nNames, (int)laserIdentifiers.size());
        for (int i = 0; i < nNamesToReturn; ++i) {
            strcpy(namesPtr[i], laserIdentifiers.at(i).substr(0, maxNBytesPerName - 1).data());
        }
        *canControlPower = 1;
        *allowMultipleChannels = 1;
        *nNamesReturned = nNamesToReturn;
    });
}

int ActivateLightSource(char* lightSourceName, char** channelNames, double* illuminationPowers, int nChannels) {
    printf("TEST\n");
    int handle = HandleExceptions([=]() {
        if (strcmp(lightSourceName, "RTC") != 0) {
            throw std::runtime_error("RTC only supports RTC lightsource");
        }

        for (int i = 0; i < nChannels; ++i) {
            std::string channelName(channelNames[i]);
            gRTC.activateLaser(channelName, illuminationPowers[i]);
        }
    });
    printf("handle %i\n",handle);
    return handle;
}

int DeactivateLightSource() {
    return HandleExceptions([=]() {
        gRTC.deactivateLasers();
    });
}

int ListDiscreteMovableComponents(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    *nNamesReturned = 0;
    return 0;
}
int ListContinuouslyMovableComponents(char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    return HandleExceptions([=]() {
        std::vector<std::string> motors = gRTC.listAvailableMotors();
        int nNamesToReturn = std::min(nNames, (int)motors.size());
        for (int i = 0; i < nNamesToReturn; ++i) {
                strcpy(namesPtr[i], motors.at(i).substr(0, maxNBytesPerName - 1).data());
        }
        *nNamesReturned = nNamesToReturn;
    });
}
int ListDiscreteMovableComponentSettings(char* discreteComponentName, char** namesPtr, int nNames, int maxNBytesPerName, int* nNamesReturned) {
    *nNamesReturned = 0;
    return 0;
}

int ListContinuouslyMovableComponentRange(char* discreteComponentName, double* minValue, double* maxValue, double* increment) {
    std::string s = discreteComponentName;
    
    *minValue = 0.0;
    *maxValue = 100.0;
    *increment = 1.0;
    if (s.find("Motor") != std::string::npos) {
        std::tuple<int,int> params = gRTC.getMotorParams(s);
        *minValue = std::get<0>(params);
        *maxValue = std::get<1>(params);
    }
    
    return 0;
}

int SetMovableComponents(int nDiscreteComponentNames, char** discreteComponentNames, char** discreteSettings,
                         int nContinuousComponentNames, char** continuousComponentNames, double* continuousSettings) {
    return HandleExceptions([=]() {
        for (int i = 0; i < nContinuousComponentNames; ++i) {
            std::string motorName = continuousComponentNames[i];
            double setting = continuousSettings[i];
            gRTC.moveMotor(motorName, setting);
        }
    });
}

int HasMotorizedStage(int* hasIt) {
    *hasIt = 0;
    return 0;
}

int MotorizedStageName(char* name, int maxNBytesPerName) {
    return -1;
}


int SupportedStageAxes(int* x, int* y, int* z) {
    return -1;
}


int GetStagePosition(double* x, double* y, double* z, int* usingHardwareAF, int* afOffset) {
    return -1;
}

int SetStagePosition(double x, double y, double z, int usingHardwareAF, int afOffset) {
    return -1;
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
