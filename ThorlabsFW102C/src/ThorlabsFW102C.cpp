#include "ThorlabsFW102C.h"

#include <algorithm>
#include <cctype>
#include <format>

#include "ImagerPluginCore/PluginManager.h"

namespace {

// Thrown when the device itself reports an error (a "Command error ..." response),
// as opposed to a communication or response-parsing problem.
class CommandError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace

ThorlabsFW102C::ThorlabsFW102C(int deviceIndex, const std::string& portName, int baudrate,
                               const std::vector<std::string>& filterNames, std::uint32_t timeoutMillis) :
    _deviceIndex(deviceIndex),
    _portName(portName),
    _baudrate(baudrate),
    _timeoutMillis(timeoutMillis),
    _filterNames(filterNames)
{
}

ThorlabsFW102C::~ThorlabsFW102C() {
}

void ThorlabsFW102C::init() {
    _serialPort.open(_portName, _baudrate, _timeoutMillis);

    // Ask the device how many positions the wheel has (6 or 12). The syntax of this
    // query depends on the firmware version: newer firmware (manual Rev J) expects
    // "pcount?", older firmware (manuals Rev C and D) expects "pcount=?".
    std::string response;
    try {
        response = _sendQuery("pcount?");
    } catch (const CommandError&) {
        response = _sendQuery("pcount=?");
    }
    try {
        _positionCount = std::stoi(response);
    } catch (const std::exception&) {
        throw std::runtime_error(std::format("Unexpected response to pcount query from ThorlabsFW102C: '{}'", response));
    }
    if (_positionCount <= 0) {
        throw std::runtime_error(std::format("ThorlabsFW102C reported an invalid position count: {}", _positionCount));
    }

    for (int pos = _positionCount + 1; pos <= static_cast<int>(_filterNames.size()); ++pos) {
        if (!_filterNames[pos - 1].empty()) {
            PluginManager::Manager().Print(std::format(
                "ThorlabsFW102C_{}: ignoring filter {} (\"{}\"), the device only has {} positions",
                _deviceIndex, pos, _filterNames[pos - 1], _positionCount));
        }
    }
}

void ThorlabsFW102C::shutdown() {
    if (_serialPort.isOpen()) {
        _serialPort.close();
    }
}

std::vector<std::shared_ptr<DiscreteMovableComponent>> ThorlabsFW102C::getDiscreteMovableComponents() {
    std::vector<std::shared_ptr<DiscreteMovableComponent>> components;

    // Show all positions of the wheel, whether or not a filter name is configured for them.
    std::vector<std::string> settings;
    for (int pos = 1; pos <= _positionCount; ++pos) {
        std::string filterName = pos <= static_cast<int>(_filterNames.size()) ? _filterNames[pos - 1] : "";
        if (filterName.empty()) {
            settings.push_back(std::format("F{}", pos));
        } else {
            settings.push_back(std::format("F{} - {}", pos, filterName));
        }
    }

    std::string fwName = std::format("ThorlabsFW102C_{}", _deviceIndex);
    components.push_back(std::make_shared<DiscreteMovableComponentFunctor>(
        fwName,
        settings,
        [this, settings](const std::string& setting) {
            auto it = std::find(settings.cbegin(), settings.cend(), setting);
            if (it == settings.cend()) {
                throw std::runtime_error("invalid filter setting: " + setting);
            }
            int position = std::distance(settings.cbegin(), it) + 1;  // positions are numbered from 1
            _setPosition(position);
        }
    ));
    return components;
}

std::string ThorlabsFW102C::_sendQuery(const std::string& cmd) {
    // Commands are terminated by a carriage return; the device echoes the command,
    // sends the answer (for queries), and finishes with a "> " prompt.
    const std::string fullCmd = cmd + "\r";
    const std::string response = _serialPort.writeAndReadUntilString(fullCmd, "> ");

    if (!response.starts_with(fullCmd)) {
        throw std::runtime_error(std::format("Unexpected response from ThorlabsFW102C to '{}': '{}'", cmd, response));
    }

    std::string body = response.substr(fullCmd.size());
    if (body.ends_with("> ")) {
        body = body.substr(0, body.size() - 2);
    } else if (body.ends_with(">")) {
        body = body.substr(0, body.size() - 1);
    }

    // trim whitespace (including the CR separating answer and prompt)
    const auto first = std::find_if(body.begin(), body.end(), [](unsigned char c) { return !std::isspace(c); });
    const auto last = std::find_if(body.rbegin(), body.rend(), [](unsigned char c) { return !std::isspace(c); }).base();
    if (first >= last) {
        return "";
    }
    body = std::string(first, last);

    if (body.starts_with("Command error")) {
        throw CommandError(std::format("ThorlabsFW102C rejected '{}': {}", cmd, body));
    }

    return body;
}

void ThorlabsFW102C::_setPosition(int position) {
    if (position < 1 || position > _positionCount) {
        throw std::runtime_error(std::format("Invalid position {} for ThorlabsFW102C (has {} positions)",
                                              position, _positionCount));
    }
    _sendQuery(std::format("pos={}", position));
}
