#pragma once

#include <string>
#include <vector>
#include <utility>

#include "ImagerPluginCore/DeviceTemplates.h"

class DummyLightSource : public LightSource {
public:
    DummyLightSource() = default;
    ~DummyLightSource() override = default;

    std::string getName() const override;
    std::vector<std::string> getChannels() const override;

    bool canControlPower() const override;
    bool allowMultipleChannelsAtOnce() const override;

    using ChannelSetting = std::pair<std::string, double>;   // channel name, illumination power
    void activate(const std::vector<ChannelSetting>& channelSettings) override;
    void deactivate() override;
};


class DummyFilterWheel : public DiscreteMovableComponent {
public:
    DummyFilterWheel() = default;
    ~DummyFilterWheel() override = default;

    std::string getName() const override;
    std::vector<std::string> getSettings() const override;

    void setTo(const std::string& setting) override;
};


class DummyPolarizer : public ContinuouslyMovableComponent {
public:
    DummyPolarizer() = default;
    ~DummyPolarizer() override = default;

    std::string getName() const override;
    double getMinValue() const override;
    double getMaxValue() const override;
    double getIncrement() const override;

    void setTo(double value) override;
};


class DummyMotorizedStage : public MotorizedStage {
public:
    DummyMotorizedStage() = default;
    ~DummyMotorizedStage() override = default;

    std::string getName() const override;
    bool supportsX() const override;
    bool supportsY() const override;
    bool supportsZ() const override;

    Position getPosition() override;
    void setPosition(Position position) override;
    bool isMoving() override {return false;}
    void stopMoving() override {;}

private:
    double _x{0.0};
    double _y{0.0};
    double _z{0.0};
    bool _hardwareAF{false};
    int _afOffset{0};
};


class DummyRobot : public Robot {
public:
    DummyRobot() = default;
    ~DummyRobot() override = default;

    std::string getName() const override;
    std::vector<RobotProgramDescription> getProgramDescriptions() const override;
    void executeProgram(const RobotProgramExecutionParams& programParams) override;
    std::variant<bool, ErrorMessage> isExecuting() const override;
    void stop() override;
};
