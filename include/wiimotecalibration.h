#pragma once

#include <cstddef>
#include <vector>

struct WiiMoteIRPoint
{
    bool visible = false;

    int rawX = 0;
    int rawY = 0;

    float x = 0.0f;
    float y = 0.0f;
};

struct WiiMoteCalibrationProfile
{
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    bool calibrated = false;
};

class WiiMoteCalibration
{
    public:
        explicit WiiMoteCalibration(std::size_t remoteCount = 0);

        void Resize(std::size_t remoteCount);
        std::size_t Size() const;

        void Reset(std::size_t remoteIndex);
        void ResetAll();

        bool IsCalibrated(std::size_t remoteIndex) const;

        const WiiMoteCalibrationProfile& GetProfile(std::size_t remoteIndex) const;
        void SetProfile(std::size_t remoteIndex, const WiiMoteCalibrationProfile& profile);

        // Applies the profile belonging to this remote.
        WiiMoteIRPoint Apply(std::size_t remoteIndex, const WiiMoteIRPoint& rawPoint) const;

    private:
        std::vector<WiiMoteCalibrationProfile> profiles;
};
