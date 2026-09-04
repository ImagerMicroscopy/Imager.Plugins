#ifndef RTC_H
#define RTC_H

#include <map>
#include <string>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "pugixml.hpp"

#include "RTCConnectionHandler.h"
#include "SCMsgParsing.h"

class RTC {
public:
    RTC();
    ~RTC() {;}

    RTC(const RTC&) = delete;
    RTC& operator=(const RTC&) = delete;

    void init();

    std::vector<std::shared_ptr<LightSource>> getLightSources();
    std::vector<std::shared_ptr<ContinuouslyMovableComponent>> getContinuouslyMovableComponents();

private:
    [[nodiscard]] std::vector<std::string> listAvailableLasers() const;
    [[nodiscard]] std::vector<std::string> listAvailableMotors() const;

    void activateLaser(const std::string& laserIdentifier, double laserPowerPercent);
    void deactivateLasers();
    void moveMotor(const std::string& motorIdentifier, double settingMm);
    std::tuple<double,double> getMotorParams(const std::string& motorIdentifier);

    std::vector<Laser> _fetchLasers();
    static std::string _generateLaserIdentifier(const Laser& laser);
    std::vector<Motor> _fetchMotors();
    static std::string _generateMotorIdentifier(const Motor& motor);

    pugi::xml_document _performRTCQuery(const pugi::xml_document& query);
    pugi::xml_document _makeEnumerateDevicesQuery();
    pugi::xml_document _makeSettingsQuery(const std::string& deviceName);

    void _performCommand(const pugi::xml_document &command);
    pugi::xml_document _performGetState(const std::string& deviceName);
    pugi::xml_document _triggerActionMessage();
    pugi::xml_document _enableLaserMessage(const std::string& laserName, bool enable);
    pugi::xml_document _setLaserPowerMessage(const std::string& laserName, int power);
    pugi::xml_document _moveMotorMessage(const std::string& motorName,int axisID, int setting);
    pugi::xml_document _getStateMessage(const std::string& deviceName);

    const int _kActionID = 999;
    // Millimetres represented by a single TIRF motor step (GenMot.0 hardware constant).
    static constexpr double kMmPerStep = 0.0025;

    static pugi::xml_document _xmlFormat(const std::string& str);

    std::map<std::string, Laser> _lasers;
    std::map<std::string, Motor> _motors;
    std::vector<Motor> _motorsVect;
    
    bool _haveInit;
    RTCConnectionHandler _connHandler;
    int _msgID;
};

#endif
