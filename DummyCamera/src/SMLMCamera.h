#ifndef SMLMCAMERA_H
#define SMLMCAMERA_H

#include <chrono>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "ImagerPluginCore/BaseCameraClass.h"

class SMLMCamera : public BaseCameraClass {
public:
    SMLMCamera();
    ~SMLMCamera() override = default;

    std::string getIdentifierStr() override { return _identifier; }
    double getFrameRate() override { return 1.0 / _exposureTime; }

private:
    using Clock = std::chrono::steady_clock;

    struct Emitter {
        double x, y;
        double amplitude;
        Clock::time_point spawnTime;
    };

    enum PropertyIDs {
        PropNoiseLevel = CameraProperty::FirstAvailablePropertyID
    };

    std::vector<CameraProperty> _derivedGetCameraProperties() override;
    void _derivedSetCameraProperties(const std::vector<CameraProperty>& properties) override;
    AcquiredImage _derivedAcquireSingleImage() override;

    void _updateEmitters(Clock::time_point now);
    void _render(std::uint16_t* data);

    static constexpr int kWidth = 512;
    static constexpr int kHeight = 512;
    static constexpr double kFwhm = 3.0;
    static constexpr double kLifetimeSeconds = 30.0;
    static constexpr double kSpawnRatePerSecond = 20.0;
    static constexpr double kBackground = 100.0;

    std::string _identifier;
    static inline int _counter = 0;

    double _exposureTime = 50.0e-3;
    double _noiseLevel = 100.0;

    std::mutex _mutex;
    std::mt19937 _prng;
    std::vector<Emitter> _emitters;
    Clock::time_point _lastUpdate;
    double _spawnBacklog = 0.0;
};

#endif
