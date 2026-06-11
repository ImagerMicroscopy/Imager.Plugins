#include "OlympusIX83.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <thread>

#define	GT_MDK_PORT_MANAGER	"msl_pm.dll"
#define CALLBACK    __stdcall

// Host-Key control IDs (Appendix "Table Ap1" of the IX3-TPC I/F spec).
// Objective hole n (1..6) maps to control ID (kObjectiveControlIdBase + n).
static constexpr int kNumObjectiveHoles      = 6;
static constexpr int kObjectiveControlIdBase = 150;   // OBSEQ1..6 == 151..156
static constexpr int kFesControlId           = 354;   // Focus escape/return button (Host Key)
static constexpr int kZcafControlId          = 358;   // Continuous-AF start button (Host Key)
// SKD display states (SKD p2): 0 = Disable, 1 = Normal, 2 = Pressed.
static constexpr int kSkdNormal  = 1;
static constexpr int kSkdPressed = 2;

// Direct (non-Host-Key) focus/ZDC panel controls: they drive the scope
// themselves and report via the 'O' operation notification, so we only un-grey
// their display with SD and don't relay them. (Confirmed accepted by SD on the
// hardware; 356 FFC rejects SD so it cannot be host-enabled and follows its
// slider. 354 FES is a Host Key and is handled separately.)
static constexpr int kDirectControlIds[] = {
    353,   // FSL  Focus slider
    355,   // FC   Focus coordinates (Z readout)
    357,   // ZOAF One-Shot AF (find focus)
    359,   // ZASL Offset (aberration) lens slider
    360,   // ZAFC Offset lens fine/coarse
};
// SD display state (SD p2): 0 = Disable, 1 = Enable.
static constexpr int kSdEnable = 1;

// -------------------------------------------------------------------------
// Optional tracing. The build never defines DEBUG and std::cout is invisible
// from a DLL inside the host, so this writes to OutputDebugString (DebugView /
// VS) AND to %TEMP%\OlympusIX83.log. Off by default; flip kVerbose to true to
// trace the command traffic and the touch-panel relay.
static constexpr bool kVerbose = true;

static void ix83Log(const std::string& line) {
    if (!kVerbose) {
        return;
    }
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);

    SYSTEMTIME st;
    GetLocalTime(&st);
    std::string stamped = std::format("[{:02}:{:02}:{:02}.{:03}] IX83: {}\n",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, line);

    OutputDebugStringA(stamped.c_str());

    char dir[MAX_PATH];
    DWORD n = GetTempPathA(MAX_PATH, dir);
    if (n > 0) {
        std::ofstream f(std::string(dir, n) + "OlympusIX83.log", std::ios::app);
        if (f) {
            f << stamped;
        }
    }
}
// -------------------------------------------------------------------------

// Unsolicited TPC notifications arrive framed inside m_Cmd (m_Rsp is empty),
// e.g. bytes "...A<B:N36.SK 153,1...". Extract the payload after the ":N<seq>"
// marker: skip the sequence digits and the delimiter (a '.'), then take the
// printable run starting at the payload keyword. Returns "" if no marker found.
static std::string extractNotificationBody(const BYTE* buf, size_t len) {
    auto printable = [](BYTE c) { return c >= 0x20 && c < 0x7f; };
    for (size_t i = 0; i + 2 < len; ++i) {
        if (buf[i] == ':' && buf[i + 1] == 'N' && isdigit(static_cast<unsigned char>(buf[i + 2]))) {
            size_t j = i + 2;
            while (j < len && isdigit(static_cast<unsigned char>(buf[j]))) ++j;   // sequence digits
            while (j < len && !isalpha(static_cast<unsigned char>(buf[j]))) ++j;  // skip delimiter(s) to keyword
            size_t k = j;
            while (k < len && printable(buf[k])) ++k;                             // payload
            return std::string(reinterpret_cast<const char*>(buf + j), k - j);
        }
    }
    return {};
}

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

//	NOTIFICATION: call back entry from SDK port manager.
//	Fires on an SDK thread for unsolicited notifications (SK, NOB, NFP, ...).
//	The notification body is delivered framed inside the MDK_MSL_CMD's m_Cmd
//	(m_Rsp is empty for notifications); extractNotificationBody pulls the payload.
int	CALLBACK NotifyCallback(
    [[maybe_unused]] ULONG		MsgId,			// Callback ID.
    [[maybe_unused]] ULONG		wParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] ULONG		lParam,			// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pv,				// Callback parameter, it depends on callback event.
    [[maybe_unused]] PVOID		pContext,		// This contains information on this call back function.
    [[maybe_unused]] PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
) {
    OlympusIX83* olympusIX83 = reinterpret_cast<OlympusIX83*>(pContext);

    std::string notification;
    if (pv != nullptr) {
        const MDK_MSL_CMD* note = reinterpret_cast<const MDK_MSL_CMD*>(pv);
        size_t len = (note->m_CmdSize > 0 && note->m_CmdSize <= MAX_COMMAND_SIZE)
                         ? note->m_CmdSize : MAX_COMMAND_SIZE;
        notification = extractNotificationBody(note->m_Cmd, len);
    }

    if (olympusIX83 != nullptr && !notification.empty()) {
        olympusIX83->postNotification(notification);
    }

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
    
    _sdkModule = LoadLibraryW(pathToOlympusDLL.c_str());
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
    _sendAndWait("SK 1");       // enable Host-Key operation notifications (setting status)
    _sendAndWait("NOB 1");      // enable objective active notification (manual rotation)
    //_sendAndWait("OPE 1");      // enter configuration mode
    _sendAndWait("OPE 0");      // leave configuration mode - needed for z-drive
    _sendAndWait("FSPD 70000,300000,60"); // set Z drive speed. parameters = initial_speed, constant_speed, accel_time


    auto [dNames,dLabels] = _getDichroicNames();
    _dichroicNames = dNames; //
    _dichroicLabels = dLabels;
    _openShutter();

    // Objective and ZDC are driven by the operator on the touch panel. After
    // login the TPC forces those buttons to be Host Keys and greys them out, so
    // we un-grey them (SKD) and start a worker thread that relays each press.
    _initHostKeyDisplay();
    _running = true;
    _notifyWorker = std::thread(&OlympusIX83::_notificationWorkerLoop, this);
}

void OlympusIX83::shutdown() {
    if (_ifData != nullptr) {
        // Stop the notification worker before tearing down the interface.
        _running = false;
        _notifyQueue.enqueue(TpcEvent{TpcEvent::ToggleZdc, 0});   // wake the worker
        if (_notifyWorker.joinable()) {
            _notifyWorker.join();
        }

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
    return _dichroicLabels;
}

void OlympusIX83::setDichroicMirror(const std::string& dichroicMirrorLabel) {
    std::lock_guard<std::mutex> lock(_commandMutex);
    auto it = std::find(_dichroicLabels.begin(), _dichroicLabels.end(), dichroicMirrorLabel);
    if (it != _dichroicLabels.end()) {
        // FOUND
        int index = static_cast<int>(std::distance(_dichroicLabels.begin(), it));
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
    std::lock_guard<std::mutex> lock(_commandMutex);
    double zPos = _getFocusPositionUM();
    return StagePosition({-1.0, -1.0, zPos, false, 0});
}

void OlympusIX83::setStagePosition(const StagePosition &pos) {
    std::lock_guard<std::mutex> lock(_commandMutex);
    double x, y, z;
    bool usingHardwareAF;
    int afOffset;
    std::tie(x, y, z, usingHardwareAF, afOffset) = pos;

    _setFocusPositionUM(z);
}

std::filesystem::path OlympusIX83::_getPathToThisDLL() {
    WCHAR path[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR) &CommandCallback, &hm) == 0) {
        int ret = GetLastError();
        throw std::runtime_error("GetModuleHandle failed to get the plugin path");
    }
    if (GetModuleFileNameW(hm, path, MAX_PATH) == 0) {
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

void OlympusIX83::_openShutter() {
    _sendAndWait("ESH1 0");
}

void OlympusIX83::_closeShutter() {
    _sendAndWait("ESH1 1");
}

int OlympusIX83::_getObjectiveHole() {
    std::string response = _sendAndWait("OB?");   // response: "OB <pos>"
    int hole = 0;
    sscanf_s(response.c_str(), "%*s %d", &hole);
    return hole;
}

// Un-grey the touch-panel controls and reflect current state. Host Keys
// (objectives, AF buttons) use SKD; direct controls (focus/offset sliders) use
// SD. Runs once during construction (single-threaded), so it takes no mutex.
void OlympusIX83::_initHostKeyDisplay() {
    _currentObjectiveHole = _getObjectiveHole();
    _zdcOn = false;
    _focusEscaped = false;

    // Host-Key buttons: objective buttons (Pressed for the one in the light
    // path, Normal for the rest; empty holes just reply with an error we ignore),
    // the Continuous-AF push-button, and the focus escape/return button.
    for (int hole = 1; hole <= kNumObjectiveHoles; ++hole) {
        int state = (hole == _currentObjectiveHole) ? kSkdPressed : kSkdNormal;
        _sendAndWait(std::format("SKD {},{}", kObjectiveControlIdBase + hole, state));
    }
    _sendAndWait(std::format("SKD {},{}", kZcafControlId, kSkdNormal));   // Continuous AF off
    _sendAndWait(std::format("SKD {},{}", kFesControlId, kSkdNormal));    // Not escaped

    // Direct controls: enable their display so the operator can use them.
    for (int id : kDirectControlIds) {
        _sendAndWait(std::format("SD {},{}", id, kSdEnable));
    }
}

// Worker thread: relays operator touch-panel presses into hardware commands.
void OlympusIX83::_notificationWorkerLoop() {
    while (_running) {
        TpcEvent ev;
        _notifyQueue.wait_dequeue(ev);
        if (!_running) {
            break;
        }
        ix83Log(std::format("worker: event kind={} hole={}", static_cast<int>(ev.kind), ev.hole));
        try {
            switch (ev.kind) {
            case TpcEvent::ChangeObjective:       _changeObjectiveViaSequence(ev.hole); break;
            case TpcEvent::ToggleZdc:             _toggleContinuousAF();                break;
            case TpcEvent::FocusEscape:           _focusEscape();                       break;
            case TpcEvent::ObjectiveDisplayUpdate: {
                std::lock_guard<std::mutex> lock(_commandMutex);
                _setObjectiveDisplay(ev.hole);
                break;
            }
            }
        } catch (const std::exception& e) {
            ix83Log(std::format("worker error: {}", e.what()));
        }
    }
}

// Move SKD highlight from the previous objective to `newHole`. Caller holds the
// command mutex. Updates _currentObjectiveHole.
void OlympusIX83::_setObjectiveDisplay(int newHole) {
    if (_currentObjectiveHole >= 1 && _currentObjectiveHole <= kNumObjectiveHoles &&
        _currentObjectiveHole != newHole) {
        _sendAndWait(std::format("SKD {},{}", kObjectiveControlIdBase + _currentObjectiveHole, kSkdNormal));
    }
    if (newHole >= 1 && newHole <= kNumObjectiveHoles) {
        _sendAndWait(std::format("SKD {},{}", kObjectiveControlIdBase + newHole, kSkdPressed));
    }
    _currentObjectiveHole = newHole;
}

// Operator tapped an objective button: run the documented objective sequence
// (BSW note Fig. 6). TPC/MMI are disabled around the action.
void OlympusIX83::_changeObjectiveViaSequence(int hole) {
    if (hole < 1 || hole > kNumObjectiveHoles) {
        return;
    }
    std::lock_guard<std::mutex> lock(_commandMutex);
    _sendAndWait("EN5 0");
    _sendAndWait(std::format("OBSEQ {}", hole));
    _setObjectiveDisplay(hole);
    _sendAndWait("EN5 1");
}

// Operator tapped the Continuous-AF button: start/stop ZDC continuous AF
// (BSW note Fig. 22).
void OlympusIX83::_toggleContinuousAF() {
    std::lock_guard<std::mutex> lock(_commandMutex);
    bool turnOn = !_zdcOn;
    _sendAndWait("EN5 0");
    _sendAndWait(turnOn ? "AF 2" : "AF 0");
    _sendAndWait(std::format("SKD {},{}", kZcafControlId, turnOn ? kSkdPressed : kSkdNormal));
    _sendAndWait("EN5 1");
    _zdcOn = turnOn;
}

// Operator tapped the Focus escape/return button. Escape drives the objective to
// its minimum position (away from the sample); the next tap returns it to the
// focus position saved at escape time. There is no documented "escape" command,
// so this is done with a focus move (FG 0 clamps at the near limit).
void OlympusIX83::_focusEscape() {
    std::lock_guard<std::mutex> lock(_commandMutex);
    _sendAndWait("EN5 0");
    if (!_focusEscaped) {
        _preEscapeZ = _getFocusPositionUM();
        _setFocusPositionUM(0.0);   // go to minimum (objective away from sample)
        _sendAndWait(std::format("SKD {},{}", kFesControlId, kSkdPressed));
        _focusEscaped = true;
    } else {
        _setFocusPositionUM(_preEscapeZ);
        _sendAndWait(std::format("SKD {},{}", kFesControlId, kSkdNormal));
        _focusEscaped = false;
    }
    _sendAndWait("EN5 1");
}

// Parse an unsolicited TPC notification and enqueue a relay event if relevant.
// Runs on the SDK callback thread: must not block or send commands.
void OlympusIX83::postNotification(const std::string& raw) {
    // Be robust to any leading framing/delimiter bytes (the protocol puts a '.'
    // between the sequence marker and the payload): start at the keyword.
    size_t start = 0;
    while (start < raw.size() && !isalpha(static_cast<unsigned char>(raw[start]))) {
        ++start;
    }
    std::string notification = raw.substr(start);
    ix83Log(std::format("postNotification: [{}]", notification));

    int id = 0;
    int state = 0;
    if (sscanf_s(notification.c_str(), "SK %d,%d", &id, &state) == 2) {
        if (state != 1) {
            ix83Log(std::format("  SK id={} state={} (release/ignored)", id, state));
            return;   // act on press (1), ignore release (0)
        }
        if (id >= kObjectiveControlIdBase + 1 && id <= kObjectiveControlIdBase + kNumObjectiveHoles) {
            ix83Log(std::format("  -> enqueue ChangeObjective hole={}", id - kObjectiveControlIdBase));
            _notifyQueue.enqueue(TpcEvent{TpcEvent::ChangeObjective, id - kObjectiveControlIdBase});
        } else if (id == kZcafControlId) {
            ix83Log("  -> enqueue ToggleZdc");
            _notifyQueue.enqueue(TpcEvent{TpcEvent::ToggleZdc, 0});
        } else if (id == kFesControlId) {
            ix83Log("  -> enqueue FocusEscape");
            _notifyQueue.enqueue(TpcEvent{TpcEvent::FocusEscape, 0});
        } else {
            ix83Log(std::format("  SK id={} not handled", id));
        }
        return;
    }

    int hole = 0;
    if (sscanf_s(notification.c_str(), "NOB %d", &hole) == 1) {
        // Nosepiece rotated manually: refresh the button highlight only.
        ix83Log(std::format("  -> enqueue ObjectiveDisplayUpdate hole={}", hole));
        _notifyQueue.enqueue(TpcEvent{TpcEvent::ObjectiveDisplayUpdate, hole});
    }
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

    ix83Log(std::format("SENT: {}", cmd.substr(0, cmd.size() - 2)));
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
    ix83Log(std::format("RECV: {}", response));
    return response;
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
