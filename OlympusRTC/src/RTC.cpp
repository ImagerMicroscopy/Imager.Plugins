#include "RTC.h"

#include <Windows.h>
#include <format>
#include <sstream>

template <typename T>
T Limit(T a, T b, T c) {
    if (a < b) {
        return b;
    }
    if (a > c) {
        return c;
    }
    return a;
}

RTC::RTC() :
    _msgID(0),
    _haveInit(false)
{}

void RTC::init() {
    if (_haveInit) {
        throw std::logic_error("RTC::init() but already done");
    }

    _connHandler.acceptConnection();
    _connHandler.receiveNextSCMessage();

    std::vector<Laser> lasers = _fetchLasers();
    for (const Laser& laser : lasers) {
        _setExternalShutter(laser,false);
        _lasers.insert({_generateLaserIdentifier(laser), laser});
    }

    std::vector<Motor> motors = _fetchMotors();
    for (const Motor& motor : motors) {
        _motors.insert({_generateMotorIdentifier(motor), motor});
    }

    _haveInit = true;
}

std::vector<std::string> RTC::listAvailableLasers() const {
    std::vector<std::string> laserIdentifiers;
    for (const auto& pair : _lasers) {
        laserIdentifiers.push_back(pair.first);
    }
    return laserIdentifiers;
}

std::vector<std::string> RTC::listAvailableMotors() const {
    std::vector<std::string> motorIdentifiers;
    for (const auto& pair : _motors) {
        motorIdentifiers.push_back(pair.first);
    }
    return motorIdentifiers;
}

void RTC::activateLaser(const std::string& laserIdentifier, double laserPower) {
    laserPower = Limit(laserPower, 0.0, 100.0);
    const Laser& laser = _lasers.at(laserIdentifier);
    int setting = int(laserPower / 100.0 * laser.maxSetting);
    if (laser.hasDiscreteSettings) {
        double minDiff = std::numeric_limits<double>::infinity();
        int closestIndex = 0;
        for (int i = 0; i < laser.transValues.size(); i++){
            double difference = std::abs(laser.transValues[i] - laserPower/1000);
            if (difference < minDiff){
                closestIndex = i;
                minDiff = difference;
            }
        }

        printf("Laser %s has discrete settings. Setting power to closest possible power:%i.",laser.wavelength.c_str(),laser.transValues[closestIndex]/100);
        setting = laser.allowablePowers[closestIndex];
    }
    
    _performCommand(_setLaserPowerMessage(laser.name, setting));
    _performCommand(_enableLaserMessage(laser.name, true));
    _connHandler.sendXMLMessage(_getStateMessage(laser.name));
    _connHandler.receiveNextSCMessage();
    _connHandler.receiveNextSCMessage();
    
}

void RTC::deactivateLasers() {
    for (const auto& it : _lasers) {
        _performCommand(_enableLaserMessage(it.second.name, false));
    }
}

void RTC::moveMotor(const std::string& motorIdentifier, double setting) {
    const Motor& motor = _motors.at(motorIdentifier);
    _performCommand(_moveMotorMessage(motor.motorName,motor.axisID + 1, int(setting)));
    _connHandler.sendXMLMessage(_getStateMessage(motor.motorName));
    _connHandler.receiveNextSCMessage();
    _connHandler.receiveNextSCMessage();
    _connHandler.receiveNextSCMessage();
}

void RTC::_setExternalShutter(const Laser& laser,bool active){
    _performRTCQuery(_setExternalShutterMessage(laser.name, active));
}

std::tuple<int,int> RTC::getMotorParams(const std::string& motorIdentifier){
    const Motor& motor = _motors.at(motorIdentifier);
    return {motor.lowVal,motor.highVal};
}

std::vector<Laser> RTC::_fetchLasers() {
    // query equipment
    pugi::xml_document equipmentDescription = _performRTCQuery(_makeEnumerateDevicesQuery());
    std::vector<std::string> laserNames = ParseLaserNamesFromDevices(equipmentDescription);

    std::vector<Laser> lasers;
    for (const auto& ln : laserNames) {
        pugi::xml_document laserDescription = _performRTCQuery(_makeSettingsQuery(ln));
        lasers.push_back(ParseLaserDetails(laserDescription));
    }

    return lasers;
}

std::string RTC::_generateLaserIdentifier(const Laser& laser) {
    constexpr const char* formatStr = "RTC_{}";
    return std::format(formatStr, laser.wavelength);
}

std::vector<Motor> RTC::_fetchMotors() {
    // query equipment
    pugi::xml_document equipmentDescription = _performRTCQuery(_makeEnumerateDevicesQuery());
    std::vector<std::string> motorNames = ParseMotorNamesFromDevices(equipmentDescription);

    std::vector<Motor> motors;
    for (const auto& mn : motorNames) {
        pugi::xml_document motorDescription = _performRTCQuery(_makeSettingsQuery(mn));
        std::vector<Motor> theseMotors = ParseMotorDetails(motorDescription);
        motors.insert(motors.end(), theseMotors.cbegin(), theseMotors.cend());
    }

    return motors;
}

std::string RTC::_generateMotorIdentifier(const Motor& motor) {
    constexpr const char* id = "Motor_{}_{}";
    return std::format(id, motor.motorName, motor.axisID);
}

pugi::xml_document RTC::_performRTCQuery(const pugi::xml_document& query) {
    _connHandler.sendXMLMessage(query);

    pugi::xml_document ack = _connHandler.receiveNextSCMessage();
    if (!IsAckMessage(ack)) {
        std::ostringstream querySS, ackSS;
        query.save(querySS); ack.save(ackSS);
        throw std::runtime_error("sent query: " + querySS.str() + "but did not receive ack: " + ackSS.str());
    }

    Sleep(5);
    pugi::xml_document response = _connHandler.receiveNextSCMessage();
    return response;
}

pugi::xml_document RTC::_makeEnumerateDevicesQuery() {
    constexpr const char* baseMessage =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">\n"
        "<Controlling>\n"
        "<Enumerate Id=\"*\" Type=\"Devices\"/>\n"
        "</Controlling>\n"
        "</PCMsg>";
    
    std::string formatted = std::format(baseMessage, _msgID);
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_makeSettingsQuery(const std::string& deviceName) {
    constexpr const char* baseMessage =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">\n"
        "<Controlling>\n"
        "<GetSetting><Device Id=\"{}\"/></GetSetting>\n"
        "</Controlling>\n"
        "</PCMsg>";
    
    std::string formatted = std::format(baseMessage, _msgID++, deviceName);
    return _xmlFormat(formatted);
}

void RTC::_performCommand(const pugi::xml_document& command) {
    
    _connHandler.sendXMLMessage(command);
    pugi::xml_document response = _connHandler.receiveNextSCMessage();
    if (!IsAckMessage(response)) {
        std::ostringstream errMsg;
        errMsg << "received non-ack for command:\n" << command;
        errMsg << "\nresponse:\n" << response;
        throw std::runtime_error(errMsg.str());
    } // Parsed successfully an experiment description

    // send goExp command
    _connHandler.sendXMLMessage(_triggerActionMessage());
    response = _connHandler.receiveNextSCMessage();
    if (!IsAckMessage(response)) {
        std::ostringstream errMsg;
        errMsg << "received non-ack for command:\n" << command;
        errMsg << "\nresponse:\n" << response;
        throw std::runtime_error(errMsg.str());
    } // Parsed successfully an experiment Command List
    Sleep(5);

    response = _connHandler.receiveNextSCMessage();
    if (!ExperimentExecuted(response)) {
        printf("Command returned error");
    }
}

pugi::xml_document RTC::_triggerActionMessage() {
    constexpr const char* baseMessage =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">\n"
        "<Controlling>\n"
        "<GoExp>{}</GoExp>\n"
        "</Controlling>\n"
        "</PCMsg>";
    
    std::string formatted = std::format(baseMessage, _msgID++, _kActionID);
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_enableLaserMessage(const std::string& laserName, bool enable) {
    constexpr const char* baseMessage =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">\n"
        "<Experiment ExpId=\"{}\">\n"
        "<Commands ComId=\"1\">\n"
        "<SetShutter Id=\"{}-Shut.0\" Par=\"Val\">\n"
        "<WaitTime>100</WaitTime>\n"
        "<State>{}</State>\n"
        "<Store>0</Store>\n"
        "</SetShutter>\n"
        "</Commands>"
        "</Experiment>"
        "</PCMsg>";

    std::string formatted = std::format(baseMessage, _msgID++, _kActionID, laserName, int(enable));
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_setLaserPowerMessage(const std::string& laserName, int power) {
    constexpr const char* baseMessage = R"(
    <?xml version="1.0" encoding="UTF-8"?>
    <PCMsg MsgId="{}">
        <Experiment ExpId="{}">
            <Commands ComId="1">
                <SetAttenuator Id="{}-Att.0" Par="Val">
                    <WaitTime>100</WaitTime>
                    <Position>{}</Position>
                    <Store>0</Store>
                </SetAttenuator>
            </Commands>
        </Experiment>
    </PCMsg>
    )";

    std::string formatted = std::format(baseMessage, _msgID++, _kActionID, laserName, power);
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_moveMotorMessage(const std::string& motorName,int axisID, int setting) {
    constexpr const char* baseMessage = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <PCMsg MsgId="{}">
            <Experiment ExpId="{}">
                <Commands ComId="1">
                    <Move Id="{}" Kind="Abs" Par="Val">
                        <WaitTime>100</WaitTime>
                        <Target{}>{}</Target{}>
                        <Store>0</Store>
                    </Move>
                </Commands>
            </Experiment>
        </PCMsg>
        )";
    
    std::string formatted = std::format(baseMessage, _msgID++, _kActionID, motorName,axisID, setting,axisID);
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_setExternalShutterMessage(const std::string& laserName, bool state) {
    constexpr const char* baseMessage =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">\n"
        "<Controlling>\n"
        "<SetDevPar Id=\"{}-Shut.0\" Type=\"External\" Val=\"{}\"/>\n"
        "</Controlling>\n"
        "</PCMsg>";
    
    std::string formatted = std::format(baseMessage, _msgID++, laserName, int(state));
    return _xmlFormat(formatted);
}

pugi::xml_document RTC::_getStateMessage(const std::string& deviceName){
    constexpr const char* baseMessage = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<PCMsg MsgId=\"{}\">"
        "<Controlling>"
        "<GetState><Device Id=\"{}\"/></GetState>"
        "</Controlling>"
        "</PCMsg>";
    std::string formatted = std::format(baseMessage, _msgID++, deviceName);
    return _xmlFormat(formatted);
}
pugi::xml_document RTC::_xmlFormat(const std::string& str) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(str.c_str(),pugi::parse_declaration);
    if (!result) {
        throw std::runtime_error("XML parse error on " + str);
    }

    return doc;
}
