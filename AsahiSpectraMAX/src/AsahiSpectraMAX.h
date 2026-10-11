#ifndef ASAHI_SPECTRA_MAX_H
#define ASAHI_SPECTRA_MAX_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class AsahiSpectraMAX : public LightSource {
public:
    // channels: (channel name, filter index on the device); filter indices are numbered from 1
    AsahiSpectraMAX(const std::string& name, const std::string& portName, uint32_t baudRate,
                       const std::vector<std::pair<std::string, int>>& channels, uint32_t timeoutMillis = 10000);
    ~AsahiSpectraMAX() override { ; }

    AsahiSpectraMAX(const AsahiSpectraMAX&) = delete;
    AsahiSpectraMAX& operator=(const AsahiSpectraMAX&) = delete;

    void initialize();
    void shutdown();

    std::string getName() const override { return _name; }
    std::vector<std::string> getChannels() const override;
    bool canControlPower() const override { return true; }
    bool allowMultipleChannelsAtOnce() const override { return false; }

    void activate(const std::vector<ChannelSetting>& channelSettings) override;
    void deactivate() override;

    void setPrintCommunication(bool print) { _serialPort.setPrintCommunication(print); }

private:
    void _sendCommand(const std::string& cmd);
    double _readLampLife();

    std::string _name;
    SerialPort _serialPort;

    std::string _portName;
    uint32_t _baudRate;
    uint32_t _timeoutMillis;

    std::vector<std::pair<std::string, int>> _channels;  // channel name, device filter index (1-based)

    bool _isInitialized{false};
};

#endif
