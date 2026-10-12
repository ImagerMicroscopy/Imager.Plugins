#include "AsahiSpectraMAX.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

#include "ImagerPluginCore/PluginManager.h"

namespace {

// Removes trailing CR/LF characters so responses can be shown in a single-line error message.
std::string stripLineEnding(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
    return s;
}

}  // namespace

AsahiSpectraMAX::AsahiSpectraMAX(const std::string& name, const std::string& portName, uint32_t baudRate,
                                       const std::vector<std::pair<std::string, int>>& channels, uint32_t timeoutMillis)
    : _name(name),
      _portName(portName),
      _baudRate(baudRate),
      _timeoutMillis(timeoutMillis),
      _channels(channels)
{
}

void AsahiSpectraMAX::initialize() {
    _serialPort.open(_portName, _baudRate, _timeoutMillis);

    double lampLife = _readLampLife();
    PluginManager::Manager().Print(std::format(
        "Asahi lamp has been on {} hours (recommended lamp life 500 hours, max lamp life 1000 hours)", lampLife));

    _isInitialized = true;
}

void AsahiSpectraMAX::shutdown() {
    if (_isInitialized) {
        // close the shutter and end remote mode (best effort: the port is closed
        // below regardless of whether the device answers)
        try {
            _sendCommand("S0");
            _sendCommand("L");
        } catch (const std::runtime_error&) {
        }

        _serialPort.close();
        _isInitialized = false;
    }
}

std::vector<std::string> AsahiSpectraMAX::getChannels() const {
    std::vector<std::string> channelNames;
    channelNames.reserve(_channels.size());
    for (const auto& c : _channels) {
        channelNames.push_back(c.first);
    }
    return channelNames;
}

void AsahiSpectraMAX::activate(const std::vector<ChannelSetting>& channelSettings) {
    if (channelSettings.empty()) {
        throw std::runtime_error("AsahiSpectraMAX: no channel setting provided");
    }

    // Only one channel can be active at a time; use the first setting.
    const auto& [channel, power] = channelSettings.front();

    auto it = std::find_if(_channels.begin(), _channels.end(),
                           [&channel](const auto& c) { return c.first == channel; });
    if (it == _channels.end()) {
        throw std::runtime_error(std::format("Invalid channel name: {}", channel));
    }
    int filterIndex = it->second;

    _sendCommand("S0");  // close shutter
    _sendCommand(std::format("F{}", filterIndex));  // set filter
    // the device accepts light intensities from 5 to 100 %
    int intensity = std::clamp(static_cast<int>(std::lround(power)), 5, 100);
    _sendCommand(std::format("LI{}", intensity));  // set power
    _sendCommand("S1");  // open shutter
}

void AsahiSpectraMAX::deactivate() {
    _sendCommand("S0");  // close shutter
}

void AsahiSpectraMAX::_sendCommand(const std::string& cmd) {
    // Commands and queries are terminated by CR LF and answered with "OK"
    // (see the remote control command list in the MAX-302 instruction manual).
    const std::string response = _serialPort.writeAndReadUntilString(cmd + "\r\n", "\n");
    if (response != "OK\r\n") {
        throw std::runtime_error(std::format("Sent '{}' to Asahi, received '{}'", cmd, stripLineEnding(response)));
    }
}

double AsahiSpectraMAX::_readLampLife() {
    // response is of the form "LIFE nnn[h]\r\n" (replies are uppercase)
    std::string response = _serialPort.writeAndReadUntilString("LIFE?\r\n", "\n");
    std::string number;
    for (char c : response) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            number += c;
        }
    }
    try {
        return std::stod(number);
    } catch (const std::exception&) {
        throw std::runtime_error(std::format("Unexpected response to LIFE? from Asahi: '{}'", stripLineEnding(response)));
    }
}
