#include "DummyCamera.h"

#include <chrono>
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
    _stars.resize(400);
    for (auto& star : _stars) {
        star.x = _randDistX(_prng);
        star.y = _randDistY(_prng);
        star.z = _randDistZ(_prng);
        star.pz = star.z;
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
    AcquiredImage image = NewRecycledImage(AcquiredImage::PixelFormat::Mono16, imageDimensions.first, imageDimensions.second);
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
    auto drawLine = [&](int x0, int y0, int x1, int y1, std::uint16_t intensity) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
                data[y0 * width + x0] = std::max(data[y0 * width + x0], intensity);
            }
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    double speed = 20.0 * _getExposureTime(); // Adjust speed based on exposure
    for (auto& star : _stars) {
        star.pz = star.z;
        star.z -= speed;

        if (star.z < 0.1) {
            star.x = _randDistX(_prng);
            star.y = _randDistY(_prng);
            star.z = 1000.0;
            star.pz = star.z;
        }

        // Project
        double fov = 500.0;
        int prev_x = static_cast<int>(star.x * fov / star.pz + width / 2.0);
        int prev_y = static_cast<int>(star.y * fov / star.pz + height / 2.0);
        int curr_x = static_cast<int>(star.x * fov / star.z + width / 2.0);
        int curr_y = static_cast<int>(star.y * fov / star.z + height / 2.0);

        std::uint16_t intensity = static_cast<std::uint16_t>(std::min(65535.0, 65535.0 * (1000.0 - star.z) / 1000.0));
        drawLine(prev_x, prev_y, curr_x, curr_y, intensity);
    }
}

AcquiredImage SimpleCamera::_derivedAcquireSingleImage() {
    return _generateNewImage();
}
