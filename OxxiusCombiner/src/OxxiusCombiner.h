#ifndef OXXIUSCOMBINER_H
#define OXXIUSCOMBINER_H

#include <string>
#include <vector>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class OxxiusCombiner : public LightSource {
public:
    enum class ModulationMode { NoModulation, DigitalModulation, AnalogModulation };

    OxxiusCombiner(const std::string& name, const std::string& portName, ModulationMode modulationMode, bool turnOffLCXOnStartupAndEnd, uint32_t baudRate = 19200, uint32_t timeoutMillis = 1000);
    ~OxxiusCombiner() override;

    OxxiusCombiner(const OxxiusCombiner&) = delete;
    OxxiusCombiner& operator=(const OxxiusCombiner&) = delete;

    void initialize();

    std::string getName() const override { return _name; }
    std::vector<std::string> getChannels() const override { return _channelNames; }
    bool canControlPower() const override { return true; }
    bool allowMultipleChannelsAtOnce() const override { return true; }

    void activate(const std::vector<ChannelSetting>& channelSettings) override;
    void deactivate() override;

    void setPrintCommunication(bool print) { _serialPort.setPrintCommunication(print); }

private:
    enum class LaserType { LBX, LCX };
    enum class RegulationMode { ConstantPower, ConstantCurrent };

    struct LaserParams {
        LaserType type;
        double maxPower;
        int wavelength;
        RegulationMode regulationMode;
        int index;
        std::string channelName;
    };

    void _initialize();
    LaserParams _parseLaserInfo(int index, const std::string& info);
    std::string _sendCommand(const std::string& cmd);
    std::string _sendCommandAndCheckResponse(const std::string& cmd);
    void _sendLaserCommand(int index, const std::string& cmd, bool ignoreResponse = false);
    void _setAOMMode();
    void _initLaser(int index, const LaserParams& params);
    std::string _queryLaserInfo(int index);
    void _setLaserPower(int index, LaserType type, RegulationMode mode, double percentage);

    std::string _name;
    SerialPort _serialPort;
    ModulationMode _modulationMode{ModulationMode::NoModulation};
    bool _turnOffLCXOnStartupAndEnd{false};
    std::string _portName;
    int _baudRate;
    int _timeoutMillis;
    std::vector<LaserParams> _lasers;
    std::vector<std::string> _channelNames;
    bool _isInitialized{false};
};

#endif // OXXIUSCOMBINER_H
