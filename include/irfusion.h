#pragma once

#include <cstddef>
#include <vector>

struct WiiMoteIRPoint;

struct FusedPoint
{
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    std::size_t contributingRemotes = 0;
};

class IRFusion
{
    public:
        virtual ~IRFusion() = default;
        virtual FusedPoint Fuse(const std::vector<WiiMoteIRPoint>& observations) const = 0;
};

class AverageFusion final : public IRFusion
{
    public:
        FusedPoint Fuse(const std::vector<WiiMoteIRPoint>& observations) const override;
};