#ifndef DUMMYCAMERA_H
#define DUMMYCAMERA_H

#include <format>
#include <thread>
#include <memory>
#include <random>
#include <vector>
#include <mutex>

#include "ImagerPluginCore/BaseCameraClass.h"

struct Star {
    double x, y, z, pz;
};

class SimpleCamera : public BaseCameraClass {
public:
    SimpleCamera();
    virtual ~SimpleCamera() { ; }

    std::string getIdentifierStr() override { return _cameraIdentifierStr; }

    double getFrameRate() override;

private:
    std::vector<CameraProperty> _derivedGetCameraProperties() override;
    void _derivedSetCameraProperties(const std::vector<CameraProperty>& properties) override;

    std::pair<int, int> _getSensorSize() const;
    AcquiredImage _generateNewImage();
    void _fillImage(std::uint16_t* data, size_t nPixels);

    AcquiredImage _derivedAcquireSingleImage() override;

    double _getExposureTime() const { return _exposureTime; }
    void _setExposureTime(const double exposureTime);

    double _exposureTime = 50.0e-3; // 50 ms

    std::string _cameraIdentifierStr;
    static inline int _camCounter = 0;

    std::mt19937 _prng;
    std::uniform_real_distribution<double> _randDistX{-1.0, 1.0};
    std::uniform_real_distribution<double> _randDistY{-1.0, 1.0};
    std::uniform_real_distribution<double> _randDistZ{0.1, 1000.0};
    std::vector<Star> _stars;
    std::mutex _mutex;
    
    void _initializeStars();
};

#endif
