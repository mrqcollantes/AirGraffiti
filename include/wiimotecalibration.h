#pragma once

#include <cstddef>
#include <vector>

// Raw IR observation from a Wii Remote.
struct WiiMoteIRPoint
{
    bool visible = false;

    int rawX = 0;
    int rawY = 0;

    float x = 0.0f;
    float y = 0.0f;
};

// Final screen position.
// Remote 0 (TOP) provides screen X, Remote 1 (LEFT) provides screen Y.
// Both remotes must currently see the IR source for this point to be valid.
struct FusedPoint
{
    bool valid = false;

    float x = 0.0f;
    float y = 0.0f;

    std::size_t contributingRemotes = 0;
};

// One synchronized observation of the same IR source by both remotes.
// topX  = raw X coordinate reported by the TOP remote.
// leftX = raw X coordinate reported by the LEFT remote.
struct WiiMoteCalibrationSample
{
    float topX = 0.0f;
    float leftX = 0.0f;
};

struct WiiMoteAxisCalibration
{
    bool calibrated = false;

    float scale = 1.0f;
    float offset = 0.0f;

    float Apply(float rawX) const
    {
        return rawX * scale + offset;
    }
};

class WiiMoteCalibration
{
public:
    WiiMoteCalibration() = default;

    void Reset();

    bool IsCalibrated() const;

    // Capture order (0=TL, 1=TR, 2=BR, 3=BL).
    bool Calculate(const std::vector<WiiMoteCalibrationSample>& samples, float canvasWidth, float canvasHeight);

    // Both raw X values are required to produce a valid point.
    FusedPoint Apply(float topRawX, float leftRawX) const;

    const WiiMoteAxisCalibration& GetTopCalibration() const;
    const WiiMoteAxisCalibration& GetLeftCalibration() const;

private:
    static bool FitAxis(const std::vector<float>& raw, const std::vector<float>& target, WiiMoteAxisCalibration& result);

    WiiMoteAxisCalibration topX;
    WiiMoteAxisCalibration leftX;
};