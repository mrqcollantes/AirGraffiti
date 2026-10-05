#pragma once

#include <cstdint>
#include <cstddef>

struct CameraPoint
{
    bool valid = false;

    // Raw camera coordinates.
    float rawX = 0.0f;
    float rawY = 0.0f;

    // Calibrated AirGraffiti canvas coordinates.
    float x = 0.0f;
    float y = 0.0f;

    float area = 0.0f;
    float brightness = 0.0f;
};

class CameraManager
{
public:
    CameraManager();
    ~CameraManager();

    bool Init();
    void Update();
    void Shutdown();

    bool IsInitialized() const;
    bool IsIRActive() const;

    const CameraPoint& GetPoint() const;

private:
    bool initialized = false;

#ifdef _WIN32
    void* socketHandle = nullptr;
#endif

    CameraPoint point;

    std::uint64_t lastPacketTimeMs = 0;

    // Simple first-pass calibration.
    //
    // Raw camera coordinates:
    //   0 ... CAMERA_WIDTH-1
    //   0 ... CAMERA_HEIGHT-1
    //
    // become:
    //   0 ... OUTPUT_WIDTH-1
    //   0 ... OUTPUT_HEIGHT-1
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    void ApplyCalibration(float rawX, float rawY);
};