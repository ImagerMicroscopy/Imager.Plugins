#include "DummyEquipment.h"

#include <format>

#include "PluginManager.h"

// --- DummyLightSource ---

std::string DummyLightSource::getName() const {
    return "Dummy Light Source";
}

std::vector<std::string> DummyLightSource::getChannels() const {
    return {"Ch1", "Ch2", "Ch3"};
}

bool DummyLightSource::canControlPower() const { 
    return true; 
}

bool DummyLightSource::allowMultipleChannelsAtOnce() const { 
    return true; 
}

void DummyLightSource::activate(const std::vector<ChannelSetting>& channelSettings) {
    std::string message = "DummyLightSource activated with channels: ";
    for (const auto& setting : channelSettings) {
        message += setting.first + "@" + std::to_string(setting.second) + "; ";
    }
    PluginManager::Manager().Print(message);
}

void DummyLightSource::deactivate() {
    PluginManager::Manager().Print("DummyLightSource deactivated");
}


// --- DummyFilterWheel ---

std::string DummyFilterWheel::getName() const {
    return "Dummy Filter Wheel";
}

std::vector<std::string> DummyFilterWheel::getSettings() const {
    return {"Filter 1", "Filter 2", "Filter 3"};
}

void DummyFilterWheel::setTo(const std::string& setting) {
    PluginManager::Manager().Print("DummyFilterWheel set to: " + setting);
}


// --- DummyPolarizer ---

std::string DummyPolarizer::getName() const {
    return "Dummy Polarizer";
}

double DummyPolarizer::getMinValue() const {
    return 0.0;
}

double DummyPolarizer::getMaxValue() const {
    return 360.0;
}

double DummyPolarizer::getIncrement() const {
    return 1.0;
}

void DummyPolarizer::setTo(double value) {
    PluginManager::Manager().Print("DummyPolarizer set to: " + std::to_string(value));
}


// --- DummyMotorizedStage ---

std::string DummyMotorizedStage::getName() const {
    return "Dummy Motorized Stage";
}

bool DummyMotorizedStage::supportsX() const { return true; }
bool DummyMotorizedStage::supportsY() const { return true; }
bool DummyMotorizedStage::supportsZ() const { return true; }

MotorizedStage::Position DummyMotorizedStage::getPosition() {
    return {_x, _y, _z, _hardwareAF, _afOffset};
}

void DummyMotorizedStage::setPosition(Position position) {
    _x = std::get<0>(position);
    _y = std::get<1>(position);
    _z = std::get<2>(position);
    _hardwareAF = std::get<3>(position);
    _afOffset = std::get<4>(position);
    PluginManager::Manager().Print("DummyMotorizedStage moved to X:" + std::to_string(_x) + 
                                   " Y:" + std::to_string(_y) + " Z:" + std::to_string(_z));
}


// --- DummyRobot ---

std::string DummyRobot::getName() const {
    return "Dummy Robot";
}

std::vector<RobotProgramDescription> DummyRobot::getProgramDescriptions() const {
    return {};
}

void DummyRobot::executeProgram(const RobotProgramExecutionParams& programParams) {
    PluginManager::Manager().Print(std::format("DummyRobot executing program: {}\n", programParams.programName()));
    for (const auto& [argName, argValue] : programParams.arguments()) { 
        std::visit([&](const auto& value) {
            PluginManager::Manager().Print(std::format("Argument: {} = {}\n", argName, value.value()));
        }, argValue);
    }
}

std::variant<bool, Robot::ErrorMessage> DummyRobot::isExecuting() const {
    return false;
}

void DummyRobot::stop() {
    PluginManager::Manager().Print("DummyRobot stopped.");
}