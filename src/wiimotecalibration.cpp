#include "wiimotecalibration.h"
#include "wiimote_config.h"

#include <algorithm>
#include <vector>

WiiMoteCalibration::WiiMoteCalibration(std::size_t remoteCount)
{
    Resize(remoteCount);
}

void WiiMoteCalibration::Resize(std::size_t remoteCount)
{
    profiles.resize(remoteCount);
}

std::size_t WiiMoteCalibration::Size() const
{
    return profiles.size();
}

void WiiMoteCalibration::Reset(std::size_t remoteIndex)
{
    if (remoteIndex >= profiles.size())
        return;

    profiles[remoteIndex] = {};
}

void WiiMoteCalibration::ResetAll()
{
    for (auto& profile : profiles)
        profile = {};
}

bool WiiMoteCalibration::IsCalibrated(std::size_t remoteIndex) const
{
    return remoteIndex < profiles.size() && profiles[remoteIndex].calibrated;
}

const WiiMoteCalibrationProfile& WiiMoteCalibration::GetProfile(
    std::size_t remoteIndex) const
{
    static const WiiMoteCalibrationProfile defaultProfile{};
    if (remoteIndex >= profiles.size())
        return defaultProfile;

    return profiles[remoteIndex];
}

void WiiMoteCalibration::SetProfile(
    std::size_t remoteIndex,
    const WiiMoteCalibrationProfile& profile)
{
    if (remoteIndex >= profiles.size())
        return;

    profiles[remoteIndex] = profile;
}

WiiMoteIRPoint WiiMoteCalibration::Apply(
    std::size_t remoteIndex,
    const WiiMoteIRPoint& rawPoint) const
{
    WiiMoteIRPoint result = rawPoint;

    if (!rawPoint.visible || remoteIndex >= profiles.size())
        return result;

    const auto& profile = profiles[remoteIndex];

    result.x = rawPoint.x * profile.scaleX + profile.offsetX;
    result.y = rawPoint.y * profile.scaleY + profile.offsetY;

    result.x = std::clamp(
        result.x,
        0.0f,
        static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH - 1));

    result.y = std::clamp(
        result.y,
        0.0f,
        static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT - 1));

    return result;
}