#ifndef OLYMPUSIX83_H
#define OLYMPUSIX83_H

#define DLL_IMPORT __declspec(dllimport)

#include <filesystem>
#include <string>

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

    OlympusIX83();
    ~OlympusIX83();

    OlympusIX83(const OlympusIX83&) = delete;
    OlympusIX83& operator=(const OlympusIX83&) = delete;

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

    std::vector<std::string> _listObjectives();
    void _setObjective(const std::string& objectiveName);

    void _openShutter();
    void _closeShutter();

    void _send(std::string cmd);
    std::string _sendAndWait(std::string cmd);

    std::vector<std::string> _dichoicLabels;
    std::vector<std::string> _dichroicNames;

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
