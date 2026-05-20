#ifndef OMICRONLEDHUB_H
#define OMICRONLEDHUB_H

#include <cstdint>
#include <memory>
#include <algorithm>
#include <tuple>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class OmicronLedHub : public LightSource {
public:
    OmicronLedHub(const std::string& name, const std::string& portName, uint32_t baudRate = 500000, uint32_t timeoutMillis = 10000);
    ~OmicronLedHub() override { ; }

    OmicronLedHub(const OmicronLedHub&) = delete;
    OmicronLedHub& operator=(const OmicronLedHub&) = delete;

    void initialize();
    void shutdown();

    std::string getName() const override { return _name; }
    std::vector<std::string> getChannels() const override { return _channelNames; }
    bool canControlPower() const override { return true; }
    bool allowMultipleChannelsAtOnce() const override { return true; }

    void activate(const std::vector<ChannelSetting>& channelSettings) override;
    void deactivate() override;

    void setPrintCommunication(bool print) { _serialPort.setPrintCommunication(print); }

private:

    void _initialize();
    std::string _sendCommand(const std::string& cmd);

    std::string _name;
    SerialPort _serialPort;

    std::string _portName;
    int _baudRate;
    int _timeoutMillis;

    int _availableChannels = 0;
    std::vector<std::string> _channelNames;
    
    bool _isInitialized{false};
};

#endif
