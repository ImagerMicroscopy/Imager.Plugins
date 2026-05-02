#ifndef PRIORPROSCANIII_H
#define PRIORPROSCANIII_H

#include <string>
#include <tuple>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class PriorProScanIII : public MotorizedStage {
public:
    PriorProScanIII(const std::string& portName, uint32_t baudRate = 9600, uint32_t timeoutMillis = 30000);
    ~PriorProScanIII() override;

    PriorProScanIII(const PriorProScanIII&) = delete;
    PriorProScanIII& operator=(const PriorProScanIII&) = delete;

    std::string getName() const override { return _name; }
    bool supportsX() const override { return true; }
    bool supportsY() const override { return true; }
    bool supportsZ() const override { return _supportsZ; }

    MotorizedStage::Position getPosition() override;
    void setPosition(MotorizedStage::Position position) override;
    bool isMoving() override;
    void stopMoving() override;

private:
    void initialize();
    std::string sendCommand(const std::string& cmd);
    void handlePriorSettingChange(const std::string& msg);
    void sendPriorCommand(const std::string& msg);
    double readPositionAxis(char axisChar);
    void determineAvailableAxes();

    std::string _name;
    SerialPort _serialPort;
    bool _supportsZ{false};
    bool _isInitialized{false};
};

#endif // PRIORPROSCANIII_H
