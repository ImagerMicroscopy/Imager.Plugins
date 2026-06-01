#include "OmicronLedHub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "ImagerPluginCore/PluginManager.h"

OmicronLedHub::OmicronLedHub(const std::string& name, const std::string& portName, uint32_t baudRate, uint32_t timeoutMillis)
    : _name(name),
      _portName(portName),
      _baudRate(baudRate),
      _timeoutMillis(timeoutMillis)
{
}

void OmicronLedHub::initialize() {
    _serialPort.open(_portName, _baudRate, _timeoutMillis);
    _initialize();
    _isInitialized = true;
}

void OmicronLedHub::shutdown() {
    if (_isInitialized) {
        deactivate();

        // restore original operating mode
        std::string gomCmd = std::format("?GOM{:04X}", _originalOperatingMode);
        std::string gomResp = _sendCommand(gomCmd);

        _serialPort.close();
        _isInitialized = false;
    }
}

void OmicronLedHub::activate(const std::vector<ChannelSetting>& channelSettings) {
    std::uint8_t channelMask = 0;    // keep track of which channels need to be activated

    for (const auto& [channel, power] : channelSettings) {
        auto it = std::find(_channelNames.begin(), _channelNames.end(), channel);
        if (it == _channelNames.end()) {
            throw std::runtime_error(std::format("Invalid channel name: {}", channel));
        }
        int channelIndex = std::distance(_channelNames.begin(), it);

        if (power > 0.0) {
            channelMask |= (1 << channelIndex); // Mark this channel for activation
        }

        std::string powerCmd = std::format("?TPP[{}]{:.1f}", channelIndex + 1, power);
        std::string response = _sendCommand(powerCmd);
        if (response != "!TPP\r") {
            throw std::runtime_error(std::format("Failed to set power for channel {}: {}", channel, response));
        }
    }

    std::string cmsResp = _sendCommand(std::format("?CMM{:02X}", channelMask));
    if (cmsResp != "!CMM>\r") {
        throw std::runtime_error(std::format("Failed to set channel mask: {}", cmsResp));
    }

    std::string activateResp = _sendCommand("?CMS1");
    if (activateResp != "!CMS1>\r") {
        throw std::runtime_error(std::format("Failed to activate: {}", activateResp));
    }
}

void OmicronLedHub::deactivate() {
    std::string cmsResp = _sendCommand("?CMMFF");
    if (cmsResp != "!CMM>\r") {
        throw std::runtime_error(std::format("Failed to set channel mask: {}", cmsResp));
    };
    std::string deactivateResp = _sendCommand("?CMS0");
    if (deactivateResp != "!CMS1>\r") {
        throw std::runtime_error(std::format("Failed to deactivate: {}", deactivateResp));
    }
}

void OmicronLedHub::_initialize() {
    int nMatched = 0;
    char buf1[64], buf2[64], buf3[64];

    // try to prevent the device from sending unsolicited messages by
    // adjusting its operating mode. Get the current mode to restore it on shutdown.
    // We attempt it multiple times in case the device is sending unsolicited messages
    // while we are trying to do this.
    bool haveGOM = false;
    for (int attempt = 0; attempt < 10; ++attempt) {
        std::string response = _sendCommand("?GOM");
        nMatched = std::sscanf(response.c_str(), "!GOM%hu\r", &_originalOperatingMode);
        if (nMatched == 1) {
            haveGOM = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!haveGOM) {
        throw std::runtime_error("Failed to get operating mode from LedHUB after multiple attempts.");
    }

    std::uint16_t newOperatingMode = _originalOperatingMode & ~(1 << 13);
    std::string gomCmd = std::format("?GOM{:04X}", newOperatingMode);
    std::string gomResp = _sendCommand(gomCmd);
    // if the system was still in ad hoc mode then the response may be garbage.
    // assume the command worked and simply clear the I/O buffers.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    _serialPort.clearBuffers();

    std::string fw = _sendCommand("?GFw|");
    nMatched = std::sscanf(fw.c_str(), "!GFw%63[^|] | %63[^|] | %63[^|]\r", buf1, buf2, buf3);
    if (nMatched != 3) {
        throw std::runtime_error(std::format("Unexpected firmware response format: {}", fw));
    }

    int deviceID = std::stoi(buf2);
    if (deviceID != 20) {
        std::string errMsg = std::format("Unexpected device ID in firmware response: {}, expected 20. ", deviceID);
        errMsg += "Full firmware response: " + fw;
        throw std::runtime_error(errMsg);
    }

    std::string spec = _sendCommand("?GSI");
    nMatched = std::sscanf(spec.c_str(), "!GSI[m%63[0-9]]0 | %63[0-9]\r", buf1, buf2);
    if (nMatched != 2) {
        throw std::runtime_error(std::format("Unexpected spec response: {}", spec));
    }
    _availableChannels = std::stoi(buf1);

    for (int i = 0; i < 8; i+=1) {
        if (!(_availableChannels & (1 << i))) {
            continue; // This channel is not available
        }
        std::string thisSpec = _sendCommand(std::format("?GSI[{}]", i+1));
        nMatched = std::sscanf(thisSpec.c_str(), "!GSI%63[0-9] | %63[0-9]\r", buf1, buf2);
        if (nMatched != 2) {
            throw std::runtime_error(std::format("Unexpected spec response for channel {}: {}", i+1, thisSpec));
        }

        std::string channelName = std::format("{}@{}mW", buf1, buf2);
        _channelNames.push_back(channelName);
    }
}

std::string OmicronLedHub::_sendCommand(const std::string& cmd) {
    std::string fullCmd = cmd + "\r";
    return _serialPort.writeAndReadUntilString(fullCmd, "\r");
}

