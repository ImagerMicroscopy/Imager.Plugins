#ifndef OLYMPUSIX83_H
#define OLYMPUSIX83_H

#define DLL_IMPORT __declspec(dllimport)

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include "Windows.h"

#include "ImagerPluginCore/DeviceTemplates.h"

#include "OlympusIX83/gt.h"
#include "blockingconcurrentqueue.h"

typedef	DLL_IMPORT	int	(*fn_MSL_PM_Initialize)();

typedef	DLL_IMPORT	int	(*fn_MSL_PM_EnumInterface)();

typedef	DLL_IMPORT	int	(*fn_MSL_PM_GetInterfaceInfo)(
	IN	int				iInterfaceIndex,	// interface index
	OUT	void			**pInterface		// interface class address location
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_OpenInterface)(
	IN	void			*pInterface			// interface class address 
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_CloseInterface)(
	IN	void			*pInterface			// interface class address 
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_ConfigInterface)(
	IN	void			*pInterface,		// interface class address 
	IN  HWND			hWnd				// window handle for setting interface
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_SendCommand)(
	IN	void			*pInterface,		// interface class address 
	IN  MDK_MSL_CMD		*cmd				// command structure.
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_RegisterCallback)(
	IN void				*pInterface,		// interface class address
	IN GT_MDK_CALLBACK	Cb_Complete,		// callback for command completion
	IN GT_MDK_CALLBACK	Cb_Notify,			// callback for notification
	IN GT_MDK_CALLBACK	Cb_Error,			// callback for error notification
	IN VOID				*pContext			// callback context.
);

class OlympusIX83 {
public:
    enum StageAxis {
        xAxis,
        yAxis,
        zAxis
    };

    using StagePosition = MotorizedStage::Position;   // x, y, z, usingHardwareAF, afOffset

    // A touch-panel Host-Key event relayed from the TPC to the worker thread.
    struct TpcEvent {
        enum Kind {
            ChangeObjective,        // operator tapped an objective button: run OBSEQ
            ToggleZdc,              // operator tapped the Continuous-AF button: flip AF
            FocusEscape,            // operator tapped the focus escape/return button
            ObjectiveDisplayUpdate  // nosepiece rotated manually (NOB): refresh display
        } kind;
        int hole;                   // objective hole position (1-based); unused for ToggleZdc
    };

    OlympusIX83();
    ~OlympusIX83() { ; }

    OlympusIX83(const OlympusIX83&) = delete;
    OlympusIX83& operator=(const OlympusIX83&) = delete;

    void shutdown();

    // Called from the SDK notify callback (SDK thread). Parses an unsolicited
    // TPC notification and, if relevant, enqueues it for the worker thread.
    // Must not block / send commands.
    void postNotification(const std::string& notification);

    std::vector<std::shared_ptr<LightSource>> getLightSources() { return {}; }
    std::vector<std::shared_ptr<DiscreteMovableComponent>> getDiscreteMovableComponents();
    std::vector<std::shared_ptr<ContinuouslyMovableComponent>> getContinuouslyMovableComponents() { return {}; }
    std::vector<std::shared_ptr<MotorizedStage>> getMotorizedStages();
    std::vector<std::shared_ptr<Robot>> getRobots() { return {}; }

private:
    bool hasMotorizedDichroic() const {return true;}
    std::vector<std::string> listDichroicMirrors() const;
    void setDichroicMirror(const std::string& dichroicMirrorName);

    virtual bool hasMotorizedStage() const { return true; }
    virtual std::vector<StageAxis> supportedStageAxes() const;
    virtual StagePosition getStagePosition();
    virtual void setStagePosition(const StagePosition& pos);

private:
    std::filesystem::path _getPathToThisDLL();

    void _logOut();
    double _getFocusPositionUM();
    void _setFocusPositionUM(double pos);

    std::pair<std::vector<std::string>,std::vector<std::string>> _getDichroicNames();
    int _getDichroicPosition();
    void _setDichroicPosition(const std::string& dichroicName);

    void _openShutter();
    void _closeShutter();

    // Host-Key relay: touch-panel-driven objective change and ZDC toggle.
    int _getObjectiveHole();
    void _initHostKeyDisplay();
    void _notificationWorkerLoop();
    void _changeObjectiveViaSequence(int hole);
    void _toggleContinuousAF();
    void _focusEscape();
    void _setObjectiveDisplay(int newHole);

    void _send(std::string cmd);
    std::string _sendAndWait(std::string cmd);

    std::vector<std::string> _dichroicLabels;
    std::vector<std::string> _dichroicNames;

    // Serializes every command transaction (caller-thread setters and the
    // worker thread both drive _sendAndWait, which shares _mslCmd).
    std::mutex _commandMutex;

    // Worker thread that runs the (blocking) Host-Key command sequences off the
    // SDK callback thread, fed by _notifyQueue.
    moodycamel::BlockingConcurrentQueue<TpcEvent> _notifyQueue;
    std::thread _notifyWorker;
    std::atomic<bool> _running{false};

    // State owned by the worker thread (initialized before the worker starts).
    int _currentObjectiveHole{0};   // 0 == unknown
    bool _zdcOn{false};
    bool _focusEscaped{false};      // focus escape/return toggle state
    double _preEscapeZ{0.0};        // focus position (um) saved at escape time

    HMODULE _sdkModule;
    fn_MSL_PM_GetInterfaceInfo	pfn_GetInterfaceInfo;
    fn_MSL_PM_Initialize		pfn_InifInterface;
    fn_MSL_PM_EnumInterface		pfn_EnumInterface;
    fn_MSL_PM_OpenInterface		pfn_OpenInterface;
    fn_MSL_PM_CloseInterface	pfn_CloseInterface;
    fn_MSL_PM_SendCommand		pfn_SendCommand;
    fn_MSL_PM_RegisterCallback	pfn_RegisterCallback;

    void* _ifData;
    MDK_MSL_CMD _mslCmd;

public:
    moodycamel::BlockingConcurrentQueue<int> _callbackQueue;
};

#endif
