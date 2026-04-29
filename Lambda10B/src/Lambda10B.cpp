#include "Lambda10B.h"

#include <format>

Lambda10B::Lambda10B(const std::string& portName, int baudrate, const std::vector<std::string>& filterNames, std::uint8_t fwSpeed) :
    _portName(portName),
    _baudrate(baudrate),
    _filterNames(filterNames),
    _fwSpeed(fwSpeed)
{
    _deviceIndex = DeviceIndex;
    DeviceIndex += 1;
}

Lambda10B::~Lambda10B() {
}

void Lambda10B::init() {
    _serialPort.open(_portName, _baudrate, 5000);

    if (_serialPort.writeByteAndReadByte(238) != 13) {
        throw std::runtime_error("Lambda10B did not respond to sync byte");
    }
    if (_serialPort.writeByteAndReadByte(253) != 13) {
        throw std::runtime_error("Lambda10B did not respond to sync byte");
    }
}

std::vector<std::shared_ptr<DiscreteMovableComponent>> Lambda10B::getDiscreteMovableComponents() {
    std::vector<std::shared_ptr<DiscreteMovableComponent>> components;
    std::string fwName = std::format("Lambda10B_{}", _deviceIndex);
    components.push_back(std::make_shared<DiscreteMovableComponentFunctor>(
        fwName,
        _filterNames,
        [this](const std::string& setting) {
            auto it = std::find(_filterNames.cbegin(), _filterNames.cend(), setting);
            if (it == _filterNames.cend()) {
                throw std::runtime_error("invalid filter setting: " + setting);
            }
            int filterIndex = std::distance(_filterNames.cbegin(), it) + 1;
            _setFilter(filterIndex);
        }
    ));
    return components;
}

void Lambda10B::_setFilter(int filterIndex) {
    std::uint8_t commandByte = (_fwSpeed << 4) | filterIndex;
    if (_serialPort.writeByteAndReadByte(commandByte) != 13) {
        throw std::runtime_error("Lambda10B did not respond to filter command");
    }
}
