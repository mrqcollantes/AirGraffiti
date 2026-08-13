#include "irfusion.h"
#include "wiimotecalibration.h"

// The AverageFusion class implements a simple averaging algorithm to
// fuse multiple WiiMoteIRPoint observations into a single FusedPoint.

FusedPoint AverageFusion::Fuse(const std::vector<WiiMoteIRPoint>& observations) const
{
    FusedPoint result;

    double sumX = 0.0;
    double sumY = 0.0;

    for (const auto& observation : observations)
    {
        if (!observation.visible)
            continue;

        sumX += observation.x;
        sumY += observation.y;
        ++result.contributingRemotes;
    }

    if (result.contributingRemotes == 0)
        return result;

    result.valid = true;
    result.x = static_cast<float>(sumX / static_cast<double>(result.contributingRemotes));
    result.y = static_cast<float>(sumY / static_cast<double>(result.contributingRemotes));

    return result;
}