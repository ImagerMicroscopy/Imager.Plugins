#include "PriorProScanIII.h"

#include <cmath>
#include <format>
#include <string>
#include <thread>

#include "ImagerPluginCore/PluginManager.h"

PriorProScanIII::PriorProScanIII(const std::string& portName, uint32_t baudRate, uint32_t timeoutMillis)
    : _name("Prior ProScan III") {
    _serialPort.open(portName, baudRate, timeoutMillis);
    initialize();
    _isInitialized = true;
}

PriorProScanIII::~PriorProScanIII() {
    if (_isInitialized) {
        stopMoving();
    }
}

void PriorProScanIII::initialize() {
    // Set compression off
    handlePriorSettingChange("COMP 0");
    // Set Z direction (negative)
    handlePriorSettingChange("ZD -1");
    // Set joystick Z direction (negative)
    handlePriorSettingChange("JZD -1");
    
    determineAvailableAxes();
}

void PriorProScanIII::determineAvailableAxes() {
    std::string response = sendCommand("?");
    // Check if Z axis is available (FOCUS = NONE means no Z axis)
    if (response.find("FOCUS = NONE") != std::string::npos) {
        _supportsZ = false;
    } else {
        _supportsZ = true;
    }
}

std::string PriorProScanIII::sendCommand(const std::string& cmd) {
    std::string fullCmd = cmd + "\r";
    std::string response = _serialPort.writeAndReadUntilString(fullCmd, "\r");
    return response;
}

void PriorProScanIII::handlePriorSettingChange(const std::string& msg) {
    std::string response = sendCommand(msg);
    if (response != "0\r") {
        throw std::runtime_error("Unexpected reply from Prior ProScan III for setting change: " + response);
    }
}

void PriorProScanIII::sendPriorCommand(const std::string& msg) {
    std::string response = sendCommand(msg);
    if (response != "R\r") {
        throw std::runtime_error("Unexpected response from Prior ProScan III: " + response);
    }
}

double PriorProScanIII::readPositionAxis(char axisChar) {
    std::string cmd = "P";
    cmd += axisChar;
    std::string response = sendCommand(cmd);
    
    size_t pos;
    double value = std::stod(response, &pos);
    return value;
}

MotorizedStage::Position PriorProScanIII::getPosition() {
    double x = readPositionAxis('X');
    double y = readPositionAxis('Y');
    double z = 0.0;
    if (_supportsZ) {
        z = readPositionAxis('Z') / 10.0; // Prior returns Z in 0.1 micron units
    }
    return {x, y, z, false, 0}; // x, y, z, usingHardwareAF, afOffset
}

void PriorProScanIII::setPosition(MotorizedStage::Position position) {
    double x = std::get<0>(position);
    double y = std::get<1>(position);
    double z = std::get<2>(position);
    
    // Round to integer positions (Prior expects integer micron values)
    int xInt = static_cast<int>(std::round(x));
    int yInt = static_cast<int>(std::round(y));
    int zInt = static_cast<int>(std::round(z * 10.0)); // Z in 0.1 micron units
    
    std::string cmd = std::format("G {},{},{}", xInt, yInt, _supportsZ ? zInt : 0);
    
    sendPriorCommand(cmd);
    
    // Wait for movement to complete
    while (isMoving()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

bool PriorProScanIII::isMoving() {
    std::string response = sendCommand("$");
    int status = std::stoi(response);
    // Check bits 0, 1, 2 for X, Y, Z movement
    return (status & (1 << 0)) || (status & (1 << 1)) || (status & (1 << 2));
}

void PriorProScanIII::stopMoving() {
    sendPriorCommand("I");
}
