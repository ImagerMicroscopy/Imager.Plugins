// WARNING: The Marzhauser Tango stage requires TWO stop bits according to the Haskell
//          reference implementation. The current SerialPort class hardcodes ONE stop bit.
//          This may cause communication issues. To fix this, SerialPort::open needs to be
//          extended to accept a stopbits parameter, and the call below should use stopbits_two.
//          Serial settings should be: 57600 baud, 8N2 (not 8N1).

#include "MarzhauserTango.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <thread>
#include <vector>

#include "ImagerPluginCore/PluginManager.h"

MarzhauserTango::MarzhauserTango(const std::string& portName, uint32_t baudRate, uint32_t timeoutMillis)
    : _name("Marzhauser Tango") {
    _serialPort.open(portName, baudRate, timeoutMillis, SerialPort::SerialFormat::f8N2);
    initialize();
    _isInitialized = true;
}

void MarzhauserTango::shutdown() {
    if (_isInitialized) {
        stopMoving();
    }
}

void MarzhauserTango::initialize() {
    // Set dimension info (enable all axes)
    sendCommand("!dim 1 1 1");
    // Disable auto-status (we will poll for move completion)
    sendCommand("!autostatus 0");

    determineAvailableAxes();
}

void MarzhauserTango::determineAvailableAxes() {
    std::string response = sendCommand("?axis");
    // Response contains axis names, e.g., "X Y Z" or "X Y"
    std::vector<std::string> axes;
    size_t start = 0;
    size_t end = response.find(' ');
    while (end != std::string::npos) {
        std::string axis = response.substr(start, end - start);
        if (!axis.empty()) {
            axes.push_back(axis);
        }
        start = end + 1;
        end = response.find(' ', start);
    }
    // Add the last axis
    if (start < response.length()) {
        std::string axis = response.substr(start);
        if (!axis.empty()) {
            axes.push_back(axis);
        }
    }

    _supportsX = std::find(axes.begin(), axes.end(), "X") != axes.end();
    _supportsY = std::find(axes.begin(), axes.end(), "Y") != axes.end();
    _supportsZ = std::find(axes.begin(), axes.end(), "Z") != axes.end();
}

std::string MarzhauserTango::sendCommand(const std::string& cmd) {
    std::string fullCmd = cmd + "\r";
    std::string response = _serialPort.writeAndReadUntilString(fullCmd, "\r");
    // Remove trailing \r if present
    if (!response.empty() && response.back() == '\r') {
        response.pop_back();
    }
    return response;
}

MotorizedStage::Position MarzhauserTango::getPosition() {
    std::string response = sendCommand("?pos");
    // Response format: "x y z" or "x y" depending on axes
    std::vector<double> coords;
    size_t start = 0;
    size_t end = response.find(' ');
    while (end != std::string::npos) {
        std::string token = response.substr(start, end - start);
        if (!token.empty()) {
            try {
                coords.push_back(std::stod(token));
            } catch (...) {
                // Ignore parsing errors
            }
        }
        start = end + 1;
        end = response.find(' ', start);
    }
    // Add the last coordinate
    if (start < response.length()) {
        std::string token = response.substr(start);
        if (!token.empty()) {
            try {
                coords.push_back(std::stod(token));
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }

    double x = coords.size() > 0 ? coords[0] : 0.0;
    double y = coords.size() > 1 ? coords[1] : 0.0;
    double z = coords.size() > 2 ? coords[2] : 0.0;

    return {x, y, z, false, 0}; // x, y, z, usingHardwareAF, afOffset
}

void MarzhauserTango::setPosition(MotorizedStage::Position position) {
    double x = std::get<0>(position);
    double y = std::get<1>(position);
    double z = std::get<2>(position);

    std::string cmd = std::format("!moa {} {} {}", x, y, z);
    sendCommand(cmd);
}

bool MarzhauserTango::isMoving() {
    std::string response = sendCommand("?statusaxis");
    // Response format: "X=M Y=M Z=S" where M=Moving, S=Stopped
    // Check if any axis is moving (contains 'M')
    return response.find('M') != std::string::npos;
}

void MarzhauserTango::stopMoving() {
    sendCommand("a -1");
}
