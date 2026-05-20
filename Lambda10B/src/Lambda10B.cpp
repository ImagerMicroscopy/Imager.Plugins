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

    _sendCommand(238);  // transfer to on-line mode
}

void Lambda10B::shutdown() {
    _sendCommand(239);  // transfer to local mode
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
            int filterIndex = std::distance(_filterNames.cbegin(), it);
            _setFilter(filterIndex);
        }
    ));
    return components;
}

void Lambda10B::_sendCommand(std::uint8_t command) {
    std::string cmd({static_cast<char>(command)});
    std::string response = _serialPort.writeAndReadUntilString(cmd, "\r");
    if (response.size() != 2) {
        std::string errMsg = std::format("Invalid response from Lambda10B to command (0x{:02X})", static_cast<unsigned int>(command));
        throw std::runtime_error(errMsg);
    }
    std::uint8_t responseByte = static_cast<std::uint8_t>(response.at(0));
    if (responseByte != command) {
        // Lambda10B echoes the byte back
        std::string errMsg = std::format("Did not get echoed byte from Lambda10B, sent (0x{:02X}, received (0x{:02X})",
            static_cast<unsigned int>(command), static_cast<unsigned int>(responseByte));
        throw std::runtime_error(errMsg);
    }
    if (response.at(1) != 13) {
        throw std::runtime_error("Did not receive CR from Lambda10B");
    }
}

void Lambda10B::_setFilter(int filterIndex) {
    std::uint8_t commandByte = (_fwSpeed << 4) | filterIndex;
    _sendCommand(commandByte);
}
