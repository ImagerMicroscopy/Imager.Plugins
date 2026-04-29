#ifndef LAMBDA10B_H
#define LAMBDA10B_H

#include <cstdint>
#include <memory>
#include <algorithm>
#include <tuple>

#include "ImagerPluginCore/DeviceTemplates.h"
#include "ImagerPluginCore/SerialPort.h"

class Lambda10B {
public:
    Lambda10B(const std::string& portName, int baudrate, const std::vector<std::string>& filterNames, std::uint8_t fwSpeed);
    ~Lambda10B();

    void init();

    Lambda10B(const Lambda10B&) = delete;
    Lambda10B& operator=(const Lambda10B&) = delete;

    std::vector<std::shared_ptr<DiscreteMovableComponent>> getDiscreteMovableComponents();

private:
    void _setFilter(int filterIndex);

    std::string _portName;
    int _baudrate;
    std::vector<std::string> _filterNames;
    std::uint8_t _fwSpeed;
    SerialPort _serialPort;
    int _deviceIndex;

    inline static int DeviceIndex = 0;
};

#endif
