#include "DummyCamera.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <format>
#include <thread>

#include "ImagerPluginCore/StandardCameraProperties.h"

SimpleCamera::SimpleCamera() : _prng(std::random_device{}()) {
    _cameraIdentifierStr = std::format("SimpleCam_{}", _camCounter);
    _camCounter += 1;
    _initializeStars();
}

void SimpleCamera::_initializeStars() {
    std::lock_guard<std::mutex> lock(_mutex);
    _stars.clear();
    _stars.reserve(650);

    // Warp stars: streak outward from the image center.
    for (int i = 0; i < 400; ++i) {
        Star star;
        star.x = _randDistX(_prng);
        star.y = _randDistY(_prng);
        star.z = _randDistZ(_prng);
        star.pz = star.z;
        star.lum = _randDistLum(_prng);
        star.size = _randDistSize(_prng);
        star.passing = false;
        _stars.push_back(star);
    }

    // Passing stars: same warp flow along the same rays from the image center,
    // but with a large lateral magnitude so they fade in far from the center
    // and streak outward from there.
    for (int i = 0; i < 250; ++i) {
        Star star;
        double angle = _randDistAngle(_prng);
        double magnitude = _randDistPassMag(_prng);
        star.x = magnitude * std::cos(angle);
        star.y = magnitude * std::sin(angle);
        star.z = _randDistPassZ(_prng);
        star.pz = star.z;
        star.lum = _randDistPassLum(_prng);
        star.size = _randDistPassSize(_prng);
        star.passing = true;
        _stars.push_back(star);
    }
}

double SimpleCamera::getFrameRate() {
    return (1.0 / _getExposureTime());
}

std::vector<CameraProperty> SimpleCamera::_derivedGetCameraProperties() {
    std::vector<CameraProperty> properties;

    CameraProperty prop(CameraProperty::ReqPropExposureTime, "Exposure time");
    prop.setNumeric(_exposureTime);
    properties.push_back(prop);

    return properties;
}

void SimpleCamera::_derivedSetCameraProperties(const std::vector<CameraProperty>& properties) {
    auto propsCopy(properties);

    CameraProperty exposureProp = properties.at(0);
    if (exposureProp.getPropertyCode() != CameraProperty::ReqPropExposureTime) {
        throw std::runtime_error("DummyCamera::_derivedSetCameraProperties() but first property not exposure time");
    }
    
     _setExposureTime(exposureProp.getValue());
}

void SimpleCamera::_setExposureTime(const double exposureTime) {
    _exposureTime = clamp(exposureTime, 10e-3, 1.0);
}

std::pair<int, int> SimpleCamera::_getSensorSize() const {
    return std::make_pair(1024, 1024);
}

AcquiredImage SimpleCamera::_generateNewImage() {
    auto endTime = std::chrono::steady_clock::now() + std::chrono::duration<double>(_exposureTime);

    std::pair<int, int> imageDimensions = _getSensorSize();
    AcquiredImage image = NewRecycledImage(AcquiredImage::PixelFormat::Mono16, imageDimensions.second, imageDimensions.first);
    std::uint16_t* dataPtr = reinterpret_cast<std::uint16_t*>(image.getData().get());
    _fillImage(dataPtr, imageDimensions.first * imageDimensions.second);

    for ( ; ; ) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = endTime - now;
        if (now >= endTime) {
            break;
        }
        std::this_thread::sleep_for(std::min(remaining, std::chrono::duration_cast<decltype(remaining)>(std::chrono::milliseconds(10))));
    }

    return image;
}

void SimpleCamera::_fillImage(std::uint16_t * data, size_t nPixels) {
    // Clear image
    std::fill(data, data + nPixels, 0);

    std::lock_guard<std::mutex> lock(_mutex);

    const int width = 1024;
    const int height = 1024;

    auto addPixel = [&](int x, int y, double value) {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return;
        }
        auto& pixel = data[y * width + x];
        pixel = static_cast<std::uint16_t>(std::min(65535.0, static_cast<double>(pixel) + value));
    };

    // Soft round dot with a quadratic falloff. Overlapping dots accumulate, so
    // crossing streaks and the crowded center build up a natural glow.
    auto drawDot = [&](double cx, double cy, double intensity, double radius) {
        if (intensity <= 0.0 || radius <= 0.0) {
            return;
        }
        const int minX = static_cast<int>(std::floor(cx - radius));
        const int maxX = static_cast<int>(std::ceil(cx + radius));
        const int minY = static_cast<int>(std::floor(cy - radius));
        const int maxY = static_cast<int>(std::ceil(cy + radius));
        const double r2 = radius * radius;
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                double d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
                if (d2 >= r2) {
                    continue;
                }
                double falloff = 1.0 - d2 / r2;
                addPixel(x, y, intensity * falloff * falloff);
            }
        }
    };

    auto lerp = [](double a, double b, double t) { return a + (b - a) * t; };

    // Streaks are chains of sub-pixel-stepped dots with interpolated
    // intensity and radius, giving a tapered, anti-aliased line. The parameter
    // interval is clipped to the frame first: stars close to the camera
    // generate long segments that are almost entirely off-screen.
    auto drawStreak = [&](double x0, double y0, double x1, double y1, double i0, double i1, double r0, double r1) {
        double tLo = 0.0, tHi = 1.0;
        const double margin = std::max(r0, r1) + 1.0;

        auto clipAxis = [&](double a, double d, double lo, double hi) {
            if (std::abs(d) < 1e-12) {
                return a >= lo && a <= hi;
            }
            double tA = (lo - a) / d;
            double tB = (hi - a) / d;
            if (tA > tB) {
                std::swap(tA, tB);
            }
            tLo = std::max(tLo, tA);
            tHi = std::min(tHi, tB);
            return true;
        };

        if (!clipAxis(x0, x1 - x0, -margin, width + margin) ||
            !clipAxis(y0, y1 - y0, -margin, height + margin) ||
            tHi < tLo) {
            return;
        }

        const double clippedLength = std::hypot(x1 - x0, y1 - y0) * (tHi - tLo);
        const int steps = std::max(1, static_cast<int>(std::ceil(clippedLength / 0.5)));
        for (int k = 0; k <= steps; ++k) {
            double t = tLo + (tHi - tLo) * k / steps;
            drawDot(lerp(x0, x1, t), lerp(y0, y1, t), lerp(i0, i1, t), lerp(r0, r1, t));
        }
    };

    // Stars are only visible on the sensor once they are close enough for the
    // projection to move them away from the image center, so key brightness to
    // that visible depth range rather than the full z range.
    const double zFade = 150.0;
    auto brightness = [&](double z) {
        return std::clamp((zFade - z) / zFade, 0.0, 1.0);
    };

    double speed = 20.0 * _getExposureTime(); // Adjust speed based on exposure
    for (auto& star : _stars) {
        star.pz = star.z;
        star.z -= speed;

        if (star.z < 0.1) {
            if (star.passing) {
                double angle = _randDistAngle(_prng);
                double magnitude = _randDistPassMag(_prng);
                star.x = magnitude * std::cos(angle);
                star.y = magnitude * std::sin(angle);
                star.z = _randDistPassSpawn(_prng);
            } else {
                star.x = _randDistX(_prng);
                star.y = _randDistY(_prng);
                star.z = 1000.0;
            }
            star.pz = star.z;
        }

        // Project
        double fov = 500.0;
        double prev_x = star.x * fov / star.pz + width / 2.0;
        double prev_y = star.y * fov / star.pz + height / 2.0;
        double curr_x = star.x * fov / star.z + width / 2.0;
        double curr_y = star.y * fov / star.z + height / 2.0;

        double prevBright = brightness(star.pz);
        double currBright = brightness(star.z);
        double intensity0 = 65535.0 * star.lum * prevBright;
        double intensity1 = 65535.0 * star.lum * currBright;
        // Stars swell as they approach the viewer; per-star size adds variety.
        double radius0 = star.size * (0.6 + 2.4 * prevBright);
        double radius1 = star.size * (0.6 + 2.4 * currBright);
        drawStreak(prev_x, prev_y, curr_x, curr_y, intensity0, intensity1, radius0, radius1);
    }
}

AcquiredImage SimpleCamera::_derivedAcquireSingleImage() {
    return _generateNewImage();
}
