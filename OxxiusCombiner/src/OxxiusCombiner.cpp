#include "OxxiusCombiner.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <sstream>
#include <string>
#include <vector>

#include "ImagerPluginCore/PluginManager.h"

OxxiusCombiner::OxxiusCombiner(const std::string& name, const std::string& portName, ModulationMode modulationMode, bool turnOffLCXOnStartupAndEnd, uint32_t baudRate, uint32_t timeoutMillis)
    : _name(name),
      _portName(portName),
      _modulationMode(modulationMode),
      _turnOffLCXOnStartupAndEnd(turnOffLCXOnStartupAndEnd),
      _baudRate(baudRate),
      _timeoutMillis(timeoutMillis)
{
}

OxxiusCombiner::~OxxiusCombiner() {
    if (_isInitialized) {
        for (const auto& params : _lasers) {
            if (params.type == LaserType::LBX) {
                _sendLaserCommand(params.index, "L=0");
                _sendLaserCommand(params.index, "DL=0");
            }
            if (params.type == LaserType::LCX && _turnOffLCXOnStartupAndEnd) {
                // Disable temperature regulation (turn off laser) for LCX lasers if configured to do so
                _sendLaserCommand(params.index, "T=0", true);
            }
        }
        // Close main shutter
        _sendCommandAndCheckResponse("SH1 0");
    }
}

void OxxiusCombiner::initialize() {
    _serialPort.open(_portName, _baudRate, _timeoutMillis);
    _initialize();
    _isInitialized = true;
}

void OxxiusCombiner::_initialize() {
    // Disable analog modulation on combiner
    _sendCommandAndCheckResponse("AS=0");
    // Open main shutter (shutter 1)
    _sendCommandAndCheckResponse("SH1=1");
    
    _setAOMMode();
    
    // Query lasers on indices 1-6
    for (int i = 1; i <= 6; ++i) {
        std::string info = _queryLaserInfo(i);
        if (!info.empty()) {
            LaserParams params = _parseLaserInfo(i, info);
            // Set regulation mode based on modulation mode
            params.regulationMode = (_modulationMode == ModulationMode::DigitalModulation) 
                ? RegulationMode::ConstantCurrent 
                : RegulationMode::ConstantPower;
            _lasers.push_back(params);
            _channelNames.push_back(params.channelName);
            _initLaser(i, params);
        }
    }
}

OxxiusCombiner::LaserParams OxxiusCombiner::_parseLaserInfo(int index, const std::string& info) {
    LaserParams params;
    params.index = index;
    params.channelName = info;
    
    // Parse info format: "LBX-wavelength-power" or "LCX-wavelength-power"
    std::istringstream iss(info);
    std::string typeStr, wavelengthStr, powerStr;
    
    if (std::getline(iss, typeStr, '-') && 
        std::getline(iss, wavelengthStr, '-') && 
        std::getline(iss, powerStr)) {
        
        params.type = (typeStr == "LBX") ? LaserType::LBX : LaserType::LCX;
        params.wavelength = std::stoi(wavelengthStr);
        params.maxPower = std::stod(powerStr);
        params.regulationMode = RegulationMode::ConstantPower; // Will be set in _initialize()
    }
    
    return params;
}

std::string OxxiusCombiner::_sendCommand(const std::string& cmd) {
    std::string fullCmd = cmd + "\n";
    return _serialPort.writeAndReadUntilString(fullCmd, "\n");
}

std::string OxxiusCombiner::_sendCommandAndCheckResponse(const std::string& cmd) {
    std::string response = _sendCommand(cmd);
    if ((response != "OK\r\n" && (response != (cmd + "\r\n")))) {
        throw std::runtime_error(std::format("Unexpected response from Oxxius: sent {} received {}", cmd, response));
    }
    return response;
}

void OxxiusCombiner::_sendLaserCommand(int index, const std::string& cmd, bool ignoreResponse) {
    std::string fullCmd = std::format("L{} {}\n", index, cmd);
    std::string response = _serialPort.writeAndReadUntilString(fullCmd, "\n");
    // Laser commands echo back the command or return OK. Unless they reply inconsistently for some commands.
    if (!ignoreResponse) {
        if (response != cmd + "\r\n" && response != "OK\r\n") {
            throw std::runtime_error(std::format("Unexpected response from Oxxius laser {}: sent {} received {}", index, cmd, response));
        }
    }
}

void OxxiusCombiner::_setAOMMode() {
    for (int aomIdx = 1; aomIdx <= 2; ++aomIdx) {
        switch (_modulationMode) {
            case ModulationMode::NoModulation:
                _sendCommandAndCheckResponse(std::format("AOM{} TTL 0", aomIdx));
                _sendCommandAndCheckResponse(std::format("AOM{} AM 0", aomIdx));
                break;
            case ModulationMode::DigitalModulation:
                _sendCommandAndCheckResponse(std::format("AOM{} TTL 1", aomIdx));
                break;
            case ModulationMode::AnalogModulation:
                _sendCommandAndCheckResponse(std::format("AOM{} AM 1", aomIdx));
                break;
        }
    }
}

void OxxiusCombiner::_initLaser(int index, const LaserParams& params) {
    switch (params.type) {
        case LaserType::LCX:
            if (_turnOffLCXOnStartupAndEnd) {
                // Disable temperature regulation (laser off until first use)
                _sendLaserCommand(index, "T=0", true);
            }
            break;
        case LaserType::LBX:
            // Turn off laser
            _sendLaserCommand(index, "DL=0");
            // Enable temperature regulation
            _sendLaserCommand(index, "T=1", true);
            
            switch (_modulationMode) {
                case ModulationMode::NoModulation:
                    _sendLaserCommand(index, "AM=0");  // Disable analog modulation
                    _sendLaserCommand(index, "TTL=0"); // Disable digital modulation
                    _sendLaserCommand(index, "CW=1", true);  // constant power mode
                    _sendLaserCommand(index, "ACC=0"); // regulate output power
                    break;
                case ModulationMode::DigitalModulation:
                    _sendLaserCommand(index, "AM=0");  // Disable analog modulation
                    _sendLaserCommand(index, "CW=0", true);  // disable CW
                    _sendLaserCommand(index, "ACC=1"); // constant current mode
                    _sendLaserCommand(index, "TTL=1"); // enable digital modulation
                    break;
                case ModulationMode::AnalogModulation:
                    _sendLaserCommand(index, "CW=0", true);  // disable CW
                    _sendLaserCommand(index, "TTL=0"); // Disable digital modulation
                    _sendLaserCommand(index, "ACC=0"); // regulate output power
                    _sendLaserCommand(index, "AM=1");  // enable analog modulation
                    break;
            }
            break;
    }
}

std::string OxxiusCombiner::_queryLaserInfo(int index) {
    std::string cmd = std::format("L{} INF?\n", index);
    std::string response = _serialPort.writeAndReadUntilString(cmd, "\n");
    
    // Check for error responses
    if (response.find("timeout") != std::string::npos || 
        response.find("Not authorized") != std::string::npos) {
        return {}; // No laser at this index
    }
    
    // Remove trailing \r\n
    while (!response.empty() && (response.back() == '\n' || response.back() == '\r')) {
        response.pop_back();
    }
    return response;
}

void OxxiusCombiner::_setLaserPower(int index, LaserType type, RegulationMode mode, double percentage) {
    double actualPower = (percentage / 100.0);
    
    // Format power as "X.YY" with one decimal digit
    int whole = static_cast<int>(actualPower);
    int fractional = static_cast<int>(std::round((actualPower - whole) * 10));
    
    std::string powerStr = std::format("{}.{}", whole, fractional);
    
    switch (type) {
        case LaserType::LBX:
            _sendLaserCommand(index, std::format("PPL{} {}", index, powerStr));
            break;
        case LaserType::LCX:
            _sendCommandAndCheckResponse(std::format("P={}", powerStr));
            break;
    }
}

void OxxiusCombiner::activate(const std::vector<ChannelSetting>& channelSettings) {
    for (const auto& [channel, power] : channelSettings) {
        auto it = std::find_if(_lasers.begin(), _lasers.end(),
            [&](const LaserParams& lp) { return lp.channelName == channel; });
        
        if (it != _lasers.end()) {
            const LaserParams& params = *it;
            
            // Turn on laser
            switch (params.type) {
                case LaserType::LCX:
                    // Enable temperature regulation in case we turned it off on startup
                    _sendLaserCommand(params.index, "T=1", true);
                    _sendLaserCommand(params.index, "DL=1");
                    break;
                case LaserType::LBX:
                    _sendLaserCommand(params.index, "L=1");
                    _sendLaserCommand(params.index, "DL=1");
                    break;
            }
            
            // Set power
            _setLaserPower(params.index, params.type, params.regulationMode, power);
        }
    }
}

void OxxiusCombiner::deactivate() {
    for (const auto& params : _lasers) {
        switch (params.type) {
            case LaserType::LCX:
                _sendLaserCommand(params.index, std::format("PPL{} 0.0", params.index));
                break;
            case LaserType::LBX:
                _sendLaserCommand(params.index, "L=0");
                _sendLaserCommand(params.index, "DL=0");
                break;
        }
    }
}
