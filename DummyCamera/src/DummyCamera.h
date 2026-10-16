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
    double lum;
    double size;
    // Warp stars have a small lateral magnitude (|x|, |y| <= 1) so they only
    // become visible near the image center. Passing stars have a large lateral
    // magnitude, so they fade in far out on the same rays from the center and
    // streak past with the same direction of motion, only slower (parallax).
    bool passing;
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
    std::uniform_real_distribution<double> _randDistLum{0.3, 1.0};
    std::uniform_real_distribution<double> _randDistSize{0.7, 1.6};
    std::uniform_real_distribution<double> _randDistPassLum{0.25, 0.7};
    std::uniform_real_distribution<double> _randDistPassSize{0.9, 1.6};
    std::uniform_real_distribution<double> _randDistPassMag{30.0, 150.0};
    std::uniform_real_distribution<double> _randDistAngle{0.0, 6.283185307179586};
    std::uniform_real_distribution<double> _randDistPassZ{0.1, 200.0};
    std::uniform_real_distribution<double> _randDistPassSpawn{150.0, 200.0};
    std::vector<Star> _stars;
    std::mutex _mutex;
    
    void _initializeStars();
};

#endif
