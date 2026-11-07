#include "SMLMCamera.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <thread>

SMLMCamera::SMLMCamera() : _prng(std::random_device{}()), _lastUpdate(Clock::now()) {
    _identifier = std::format("SMLMCam_{}", _counter);
    _counter += 1;
}

std::vector<CameraProperty> SMLMCamera::_derivedGetCameraProperties() {
    std::lock_guard<std::mutex> lock(_mutex);

    CameraProperty exposure(CameraProperty::ReqPropExposureTime, "Exposure time");
    exposure.setNumeric(_exposureTime);

    CameraProperty noise(PropNoiseLevel, "Noise level");
    noise.setNumeric(_noiseLevel);

    return {exposure, noise};
}

void SMLMCamera::_derivedSetCameraProperties(const std::vector<CameraProperty>& properties) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& prop : properties) {
        switch (prop.getPropertyCode()) {
            case CameraProperty::ReqPropExposureTime:
                _exposureTime = std::clamp(prop.getValue(), 10e-3, 1.0);
                break;
            case PropNoiseLevel:
                _noiseLevel = std::clamp(prop.getValue(), 0.0, 5000.0);
                break;
            default:
                break;
        }
    }
}

void SMLMCamera::_updateEmitters(Clock::time_point now) {
    const auto lifetime = std::chrono::duration<double>(kLifetimeSeconds);
    std::erase_if(_emitters, [&](const Emitter& e) { return now - e.spawnTime >= lifetime; });

    const double elapsed = std::chrono::duration<double>(now - _lastUpdate).count();
    _lastUpdate = now;
    _spawnBacklog += std::min(elapsed, kLifetimeSeconds) * kSpawnRatePerSecond;

    std::uniform_real_distribution<double> posX(2.0, kWidth - 3.0);
    std::uniform_real_distribution<double> posY(2.0, kHeight - 3.0);
    std::uniform_real_distribution<double> amplitude(400.0, 2000.0);
    while (_spawnBacklog >= 1.0) {
        _emitters.push_back({posX(_prng), posY(_prng), amplitude(_prng), now});
        _spawnBacklog -= 1.0;
    }
}

void SMLMCamera::_render(std::uint16_t* data) {
    std::vector<double> frame(static_cast<size_t>(kWidth) * kHeight, kBackground);

    const double sigma = kFwhm / (2.0 * std::sqrt(2.0 * std::log(2.0)));
    const double inv2s2 = 1.0 / (2.0 * sigma * sigma);
    const int radius = static_cast<int>(std::ceil(3.0 * sigma));

    for (const auto& e : _emitters) {
        const int cx = static_cast<int>(std::round(e.x));
        const int cy = static_cast<int>(std::round(e.y));
        for (int y = std::max(0, cy - radius); y <= std::min(kHeight - 1, cy + radius); ++y) {
            const double dy = y - e.y;
            for (int x = std::max(0, cx - radius); x <= std::min(kWidth - 1, cx + radius); ++x) {
                const double dx = x - e.x;
                frame[static_cast<size_t>(y) * kWidth + x] += e.amplitude * std::exp(-(dx * dx + dy * dy) * inv2s2);
            }
        }
    }

    std::normal_distribution<double> noise(0.0, std::max(_noiseLevel, 1e-9));
    const bool addNoise = _noiseLevel > 0.0;
    for (size_t i = 0; i < frame.size(); ++i) {
        const double v = addNoise ? frame[i] + noise(_prng) : frame[i];
        data[i] = static_cast<std::uint16_t>(std::clamp(v, 0.0, 65535.0));
    }
}

AcquiredImage SMLMCamera::_derivedAcquireSingleImage() {
    double exposure;
    AcquiredImage image = NewRecycledImage(AcquiredImage::PixelFormat::Mono16, kHeight, kWidth);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        exposure = _exposureTime;
        _updateEmitters(Clock::now());
        _render(reinterpret_cast<std::uint16_t*>(image.getData().get()));
    }
    std::this_thread::sleep_for(std::chrono::duration<double>(exposure));
    return image;
}
