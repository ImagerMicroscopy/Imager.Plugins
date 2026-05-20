#ifndef MARZHAUSERTANGO_H
#define MARZHAUSERTANGO_H

#include <string>
#include <vector>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class MarzhauserTango : public MotorizedStage {
public:
    MarzhauserTango(const std::string& portName, uint32_t baudRate = 57600, uint32_t timeoutMillis = 30000);
    ~MarzhauserTango() override { ; }

    MarzhauserTango(const MarzhauserTango&) = delete;
    MarzhauserTango& operator=(const MarzhauserTango&) = delete;

    void shutdown();

    std::string getName() const override { return _name; }
    bool supportsX() const override { return _supportsX; }
    bool supportsY() const override { return _supportsY; }
    bool supportsZ() const override { return _supportsZ; }

    MotorizedStage::Position getPosition() override;
    void setPosition(MotorizedStage::Position position) override;
    bool isMoving() override;
    void stopMoving() override;

private:
    void initialize();
    void determineAvailableAxes();
    std::string sendCommand(const std::string& cmd);

    std::string _name;
    SerialPort _serialPort;
    bool _supportsX{false};
    bool _supportsY{false};
    bool _supportsZ{false};
    bool _isInitialized{false};
};

#endif // MARZHAUSERTANGO_H
