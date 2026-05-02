#include "OlympusIX83.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>

#define	GT_MDK_PORT_MANAGER	"msl_pm.dll"
#define CALLBACK    __stdcall

std::string extractFirstTwo(const std::string& response);
std::pair<std::string, std::string> parseResponse(const std::string& response);

int CALLBACK CommandCallback(
    [[maybe_unused]] ULONG		MsgId,			// Callback ID.
    [[maybe_unused]] ULONG		wParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] ULONG		lParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pv,				// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pContext,		// This contains information on this call back function.
    [[maybe_unused]] PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
) {
    OlympusIX83* olympusIX83 = reinterpret_cast<OlympusIX83*>(pContext);

    olympusIX83->_callbackQueue.enqueue(0);

    return 0;
}

//	NOTIFICATION: call back entry from SDK port manager
int	CALLBACK NotifyCallback(
    [[maybe_unused]] ULONG		MsgId,			// Callback ID.
    [[maybe_unused]] ULONG		wParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] ULONG		lParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pv,				// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pContext,		// This contains information on this call back function.
    [[maybe_unused]] PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
) {
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);

    return 0;
}

int	CALLBACK ErrorCallback(
    [[maybe_unused]] ULONG		MsgId,			// Callback ID.
    [[maybe_unused]] ULONG		wParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] ULONG		lParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pv,				// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pContext,		// This contains information on this call back function.
    [[maybe_unused]] PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
) {

    return 0;
}



OlympusIX83::OlympusIX83() :
    _sdkModule(nullptr),
    _ifData(nullptr)
 {
    // The Olympus SDK will dynamically load a number of libraries but it will only
    // look in the directory containing the exe. So the dlls need to be located there.
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::filesystem::path exePath(path);
    std::filesystem::path pathToOlympusDLL = exePath.remove_filename() / GT_MDK_PORT_MANAGER;
    std::string thePath = pathToOlympusDLL.string();

    _sdkModule = LoadLibrary(thePath.c_str());
    if (_sdkModule == nullptr) {
        throw std::runtime_error("could not load Olympus library");
    }

    std::vector<void*> functions;

    //-----------------------------------------------------
    // get function addresses
    pfn_InifInterface = (fn_MSL_PM_Initialize)GetProcAddress(_sdkModule, "MSL_PM_Initialize");
    functions.push_back((void*)pfn_InifInterface);

    pfn_EnumInterface = (fn_MSL_PM_EnumInterface)GetProcAddress(_sdkModule, "MSL_PM_EnumInterface");
    functions.push_back((void*)pfn_EnumInterface);

    pfn_GetInterfaceInfo = (fn_MSL_PM_GetInterfaceInfo)GetProcAddress(_sdkModule, "MSL_PM_GetInterfaceInfo");
    functions.push_back((void*)pfn_GetInterfaceInfo);

    pfn_OpenInterface = (fn_MSL_PM_OpenInterface)GetProcAddress(_sdkModule, "MSL_PM_OpenInterface");
    functions.push_back((void*)pfn_OpenInterface);

    pfn_CloseInterface = (fn_MSL_PM_CloseInterface)GetProcAddress(_sdkModule, "MSL_PM_CloseInterface");
    functions.push_back((void*)pfn_CloseInterface);

    pfn_SendCommand = (fn_MSL_PM_SendCommand)GetProcAddress(_sdkModule, "MSL_PM_SendCommand");
    functions.push_back((void*)pfn_SendCommand);

    pfn_RegisterCallback = (fn_MSL_PM_RegisterCallback )GetProcAddress(_sdkModule, "MSL_PM_RegisterCallback");
    functions.push_back((void*)pfn_RegisterCallback);

    if (std::any_of(functions.begin(), functions.end(), [](const void* f) {
        return (f == nullptr);
    })) {
        throw std::runtime_error("can't open Olympus function(s)");
    }

    int err = (*pfn_InifInterface)();
    if (err) {
        throw std::runtime_error("can't initialize MSL_PM");
    }
    
    int count = 0;
    for (int i = 0; i < 20; ++i) {
        count = (*pfn_EnumInterface)();
        if (count > 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (count == 0) {
        throw std::runtime_error("no IX83 interfaces");
    }

    err = (*pfn_GetInterfaceInfo)(0, &_ifData);
    if (!err || (_ifData == nullptr)) {
        throw std::runtime_error("can't get IX83 interface info");
    }

    bool result = (*pfn_OpenInterface)(_ifData);
    if (!result) {
        throw std::runtime_error("couldn't open IX83 interface");
    }

    result = (*pfn_RegisterCallback)(_ifData, CommandCallback, NotifyCallback, ErrorCallback, this);
    if (!result) {
        throw std::runtime_error("couldn't register IX83 callbacks");
    }

    _sendAndWait("L 1,0");      // log in
    _sendAndWait("EN6 1,1");    // enable the jog wheel
    _sendAndWait("EN5 1");      // enable TPC
    //_sendAndWait("OPE 1");      // enter configuration mode
    _sendAndWait("OPE 0");      // leave configuration mode - needed for z-drive
    _sendAndWait("FSPD 70000,300000,60"); // set Z drive speed. parameters = initial_speed, constant_speed, accel_time


    auto [dNames,dLabels] = _getDichroicNames(); 
    _dichroicNames = dNames; // 
    _dichoicLabels = dLabels;
    _openShutter();
}

OlympusIX83::~OlympusIX83() {
    if (_ifData != nullptr) {
        _closeShutter();
        _logOut();

        (*pfn_CloseInterface)(_ifData);
        _ifData = nullptr;
    }
}

std::vector<std::shared_ptr<DiscreteMovableComponent>> OlympusIX83::getDiscreteMovableComponents() {
    std::vector<std::shared_ptr<DiscreteMovableComponent>> components;
    if (hasMotorizedDichroic()) {
        const std::vector<std::string> dichroicNames = listDichroicMirrors();
        components.push_back(std::make_shared<DiscreteMovableComponentFunctor>(
            "Dichroic Mirror",
            dichroicNames,
            [this](const std::string& setting) {
                setDichroicMirror(setting);
            }
        ));
    }
    return components;
}

std::vector<std::shared_ptr<MotorizedStage>> OlympusIX83::getMotorizedStages() {
    std::vector<std::shared_ptr<MotorizedStage>> stages;
    if (hasMotorizedStage()) {
        stages.push_back(std::make_shared<MotorizedStageFunctor>(
            "Focus",
            false,  // supportsX
            false,  // supportsY
            true,   // supportsZ
            [this]() { return getStagePosition(); },
            [this](const StagePosition& pos) { setStagePosition(pos); },
            [this]() { return false; },
            [this]() {}
        ));
    }
    return stages;
}

std::vector<std::string> OlympusIX83::listDichroicMirrors() const {
    return _dichoicLabels;
}

void OlympusIX83::setDichroicMirror(const std::string& dichroicMirrorLabel) {
    auto it = std::find(_dichoicLabels.begin(), _dichoicLabels.end(), dichroicMirrorLabel);
    if (it != _dichoicLabels.end()) {
        // FOUND
        int index = static_cast<int>(std::distance(_dichoicLabels.begin(), it));
        _setDichroicPosition(_dichroicNames[index]);
    } else {
        // NOT FOUND
        _setDichroicPosition(_dichroicNames[0]);
    }
    
}

std::vector<OlympusIX83::StageAxis> OlympusIX83::supportedStageAxes() const {
    return std::vector<StageAxis>({zAxis});
}

OlympusIX83::StagePosition OlympusIX83::getStagePosition() {
    double zPos = _getFocusPositionUM();
    return StagePosition({-1.0, -1.0, zPos, false, 0});
}

void OlympusIX83::setStagePosition(const StagePosition &pos) {
    double x, y, z;
    bool usingHardwareAF;
    int afOffset;
    std::tie(x, y, z, usingHardwareAF, afOffset) = pos;

    _setFocusPositionUM(z);
}

std::filesystem::path OlympusIX83::_getPathToThisDLL() {
    char path[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR) &CommandCallback, &hm) == 0) {
        int ret = GetLastError();
        throw std::runtime_error("GetModuleHandle failed to get the plugin path");
    }
    if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
        int ret = GetLastError();
        throw std::runtime_error("GetModuleFileName failed to get the plugin path");
    }

    std::filesystem::path fsPath(path);
    return fsPath;
}

void OlympusIX83::_logOut() {
    _sendAndWait("L 0,0");
}

double OlympusIX83::_getFocusPositionUM()
{
    std::string response = _sendAndWait("FP?");
    
    int i = 0;
    sscanf_s(response.c_str(), "%*s %d", &i);

    return ((double)i / 100.0);
}

void OlympusIX83::_setFocusPositionUM(double pos) {
    int zPos = pos * 100.0;
    char buf[64];
    sprintf_s(buf, sizeof(buf), "FG %d", zPos);
    _sendAndWait(buf);
}

std::pair<std::vector<std::string>,std::vector<std::string>> OlympusIX83::_getDichroicNames() {
    if (!hasMotorizedDichroic()) {
        return std::make_pair(std::vector<std::string>(),std::vector<std::string>());
    }

    std::vector<std::string> dichroicMirrorsNames;
    std::vector<std::string> dichroicMirrorsLabels;
    for (size_t i = 0; i < 8; i += 1) {
        char buf[64];
        sprintf_s(buf, sizeof(buf), "GMU1 %d", (int)i+1);
        std::string response = _sendAndWait(buf);
        // Example response:
        // GMU1 8,U-FF,TIRF_M4,TIRF_M4
        // GMU1 8, should go to name and then extract 8-TIRF_M4 as label
        auto [name,label] = parseResponse(response);
        dichroicMirrorsLabels.push_back(label);
        dichroicMirrorsNames.push_back(name);
    }
    return std::make_pair(dichroicMirrorsNames,dichroicMirrorsLabels);
}

int OlympusIX83::_getDichroicPosition() {
    std::string response = _sendAndWait("MU1?");
    int i = 0;
    sscanf_s(response.c_str(), "%*s %d", &i);
    return (i);
}

void OlympusIX83::_setDichroicPosition(const std::string& dichroicName) {
    auto it = std::find(_dichroicNames.begin(), _dichroicNames.end(), dichroicName);
    if (it == _dichroicNames.end()) {
        throw std::logic_error("dichroic " + dichroicName + " not found");
    }
    
    int idx = std::distance(_dichroicNames.begin(), it);
    
    std::string msg = std::format("MU1 {}", idx + 1);
    _sendAndWait(msg);
}

std::vector<std::string> OlympusIX83::_listObjectives() {
    std::vector<std::string> objectives;
    for (size_t i = 0; i < 6; i += 1) {
        char buf[64];
        sprintf_s(buf, sizeof(buf), "GOB %d", (int)i+1);
        std::string response = _sendAndWait(buf);
        objectives.push_back(extractFirstTwo(response));
    }
    return objectives;
}

void OlympusIX83::_setObjective(const std::string& objectiveName) {
    auto dmList = _listObjectives();
    auto it = std::find(dmList.cbegin(), dmList.cend(), objectiveName);
    if (it == dmList.cend()) {
        throw std::runtime_error("Objective mirror not found");
    }
    int idx = std::distance(dmList.cbegin(), it);
    char buf[64];
    sprintf_s(buf, sizeof(buf), "OB %d", idx + 1);
    std::string response = _sendAndWait(buf);
    if (response.find("OB +") == std::string::npos) {
        throw std::runtime_error("error when changing objective");
    }
}

void OlympusIX83::_openShutter() {
    _sendAndWait("ESH1 0");
}

void OlympusIX83::_closeShutter() {
    _sendAndWait("ESH1 1");
}

void OlympusIX83::_send(std::string cmd) {
    memset(&_mslCmd, 0, sizeof(_mslCmd));

    cmd += "\r\n";

    if ((cmd.size() + 1) > sizeof(_mslCmd.m_Cmd)) {   // add one for the trailing nil
        throw std::runtime_error("command " + cmd + " too long");
    }

    memcpy(_mslCmd.m_Cmd, cmd.c_str(), cmd.size() + 1);
    _mslCmd.m_CmdSize = static_cast<DWORD>(cmd.size());
	_mslCmd.m_Callback = CommandCallback;
	_mslCmd.m_Context = nullptr;		// this pointer passed by pv
	_mslCmd.m_Timeout = 10000;	// (ms)
	_mslCmd.m_Sync = FALSE;
	_mslCmd.m_Command = TRUE;		// TRUE: Command , FALSE: it means QUERY form ('?').

    bool result = (*pfn_SendCommand)(_ifData, &_mslCmd);
    if (!result) {
        throw std::runtime_error("command " + cmd + " did not send successfully");
    }

#ifdef DEBUG
    std::cout << "\x1B[31mIX83: SENT: \x1B[0m" << cmd.substr(0, cmd.size() - 2) << std::endl;
#endif
}

std::string OlympusIX83::_sendAndWait(std::string cmd) {
    int dummy;
    while (_callbackQueue.try_dequeue(dummy)) {
        // clear the queue (should only happen if there were errors)
        ;
    }

    _send(cmd);

    bool haveIt = _callbackQueue.wait_dequeue_timed(dummy, std::chrono::seconds(15));
    if (!haveIt) {
        std::string errorResponse = "IX83 timeout waiting for response to command ";
        errorResponse += (char*)(_mslCmd.m_Cmd);
        throw std::runtime_error(errorResponse);
    }

    size_t nBytesInResponse = _mslCmd.m_RspSize;
    std::string response(reinterpret_cast<char*>(_mslCmd.m_Rsp), nBytesInResponse);
#ifdef DEBUG
    std::cout << "\x1B[32mIX83: RECEIVED: \x1B[0m" << response << std::endl;
#endif
    return response;
}

std::string extractFirstTwo(const std::string& response) {
    // find the space after "GOB"
    auto spacePos = response.find(' ');
    if (spacePos == std::string::npos) return "";

    // get everything after the space
    std::string rest = response.substr(spacePos + 1);

    // find first comma
    auto comma1 = rest.find(',');
    if (comma1 == std::string::npos) return "";

    // extract first token
    std::string first = rest.substr(0, comma1);

    // find second comma (if any)
    auto comma2 = rest.find(',', comma1 + 1);
    // extract second token (up to next comma or end of string)
    std::string second = rest.substr(
        comma1 + 1,
        (comma2 == std::string::npos ? std::string::npos : comma2 - comma1 - 1)
    );

    return first + "_" + second;
}

std::pair<std::string, std::string> parseResponse(const std::string& response) {
    std::vector<std::string> parts;
    std::stringstream ss(response);
    std::string item;

    // Split by comma
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }

    if (parts.size() < 3) {
        throw std::invalid_argument("Input does not have enough parts");
    }

    // Extract the first part and second part
    std::string firstPart = parts[0];  // e.g., "GMU1 8"

    // Extract the number from the first part (e.g., "8" from "GMU1 8")
    std::istringstream iss(firstPart);
    std::string token;
    std::string number;
    while (iss >> token) {
        number = token;  // The last token will be the number
    }

    std::string secondPart = number + "-" + parts[2];  // e.g., "8-TIRF_M4"

    return {firstPart, secondPart}; //name,label
}
