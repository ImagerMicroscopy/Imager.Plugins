#ifndef THORLABS_FW102C_H
#define THORLABS_FW102C_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class ThorlabsFW102C {
public:
    // filterNames: filter names by position; element i holds the name for position i+1
    // (positions are numbered from 1); an empty string means no name is configured.
    ThorlabsFW102C(int deviceIndex, const std::string& portName, int baudrate,
                   const std::vector<std::string>& filterNames, std::uint32_t timeoutMillis = 30000);
    ~ThorlabsFW102C();

    void init();
    void shutdown();
    void setPrintCommunication(bool doIt) { _serialPort.setPrintCommunication(doIt); }

    ThorlabsFW102C(const ThorlabsFW102C&) = delete;
    ThorlabsFW102C& operator=(const ThorlabsFW102C&) = delete;

    std::vector<std::shared_ptr<DiscreteMovableComponent>> getDiscreteMovableComponents();

private:
    std::string _sendQuery(const std::string& cmd);
    void _setPosition(int position);

    int _deviceIndex;
    std::string _portName;
    int _baudrate;
    std::uint32_t _timeoutMillis;
    std::vector<std::string> _filterNames;
    int _positionCount = 0;  // number of wheel positions, as reported by the device (6 or 12)
    SerialPort _serialPort;
};

#endif
