#include "DummyCamera.h"

#include <chrono>
#include <cstdlib>
#include <format>
#include <thread>

#include "ImagerPluginCore/StandardCameraProperties.h"

SimpleCamera::SimpleCamera() {
    _cameraIdentifierStr = std::format("SimpleCam_{}", _camCounter);
    _camCounter += 1;
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
    for (size_t i = 0; i < nPixels; i++) {
        data[i] = static_cast<std::uint16_t>(std::rand());
    }
}

AcquiredImage SimpleCamera::_derivedAcquireSingleImage() {
    return _generateNewImage();
}
