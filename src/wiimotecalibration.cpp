#include "wiimotecalibration.h"

#include <cmath>

namespace
{
    constexpr float EPSILON = 0.000001f;
}

void WiiMoteCalibration::Reset()
{
    topX = {};
    leftX = {};
}

bool WiiMoteCalibration::IsCalibrated() const
{
    return topX.calibrated && leftX.calibrated;
}

bool WiiMoteCalibration::Calculate(
    const std::vector<WiiMoteCalibrationSample>& samples,
    float canvasWidth, float canvasHeight)
{
    Reset();

    if (samples.size() < 2 || canvasWidth <= 0.0f || canvasHeight <= 0.0f)
        return false;

    std::vector<float> topRaw;
    std::vector<float> leftRaw;
    std::vector<float> targetX;
    std::vector<float> targetY;

    topRaw.reserve(samples.size());
    leftRaw.reserve(samples.size());
    targetX.reserve(samples.size());
    targetY.reserve(samples.size());

    // The samples are captured in screen-corner order:
    // 0 = TL, 1 = TR, 2 = BR, 3 = BL.
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        float x = 0.0f;
        float y = 0.0f;

        switch (i)
        {
            case 0: // top-left
                x = 0.0f;
                y = 0.0f;
                break;

            case 1: // top-right
                x = canvasWidth - 1.0f;
                y = 0.0f;
                break;

            case 2: // bottom-right
                x = canvasWidth - 1.0f;
                y = canvasHeight - 1.0f;
                break;

            case 3: // bottom-left
                x = 0.0f;
                y = canvasHeight - 1.0f;
                break;

            default:
                return false;
        }

        topRaw.push_back(samples[i].topX);
        leftRaw.push_back(samples[i].leftX);
        targetX.push_back(x);
        targetY.push_back(y);
    }

    const bool topSolved = FitAxis(topRaw, targetX, topX);
    const bool leftSolved = FitAxis(leftRaw, targetY, leftX);

    if (!topSolved || !leftSolved)
    {
        Reset();
        return false;
    }

    return true;
}

bool WiiMoteCalibration::FitAxis(
    const std::vector<float>& raw,
    const std::vector<float>& target,
    WiiMoteAxisCalibration& result)
{
    result = {};

    if (raw.size() != target.size() || raw.size() < 2)
        return false;

    // target = raw * scale + offset
    double sumRaw = 0.0;
    double sumTarget = 0.0;
    double sumRawSquared = 0.0;
    double sumRawTarget = 0.0;

    for (std::size_t i = 0; i < raw.size(); ++i)
    {
        const double r = raw[i];
        const double t = target[i];

        sumRaw += r;
        sumTarget += t;
        sumRawSquared += r * r;
        sumRawTarget += r * t;
    }

    const double count = static_cast<double>(raw.size());
    const double denominator = count * sumRawSquared - sumRaw * sumRaw;

    if (std::fabs(denominator) < EPSILON)
        return false;

    const double scale = (count * sumRawTarget - sumRaw * sumTarget) / denominator;
    const double offset = (sumTarget - scale * sumRaw) / count;

    result.calibrated = true;
    result.scale = static_cast<float>(scale);
    result.offset = static_cast<float>(offset);

    return true;
}

FusedPoint WiiMoteCalibration::Apply(float topRawX, float leftRawX) const
{
    FusedPoint result;

    if (!IsCalibrated())
        return result;

    result.valid = true;
    result.x = topX.Apply(topRawX);
    result.y = leftX.Apply(leftRawX);
    result.contributingRemotes = 2;

    return result;
}

const WiiMoteAxisCalibration& WiiMoteCalibration::GetTopCalibration() const
{
    return topX;
}

const WiiMoteAxisCalibration& WiiMoteCalibration::GetLeftCalibration() const
{
    return leftX;
}