#pragma once

namespace WiiMoteConfig
{
    // Maximum number of Wii Remotes supported by wiiuse.
    // Current tracking method uses exactly two:
    //   Remote 0 = TOP
    //   Remote 1 = LEFT
    constexpr int MAX_WIIMOTES = 5;

    // Wiiuse IR coordinate space.
    constexpr int IR_WIDTH = 1024;
    constexpr int IR_HEIGHT = 768;

    constexpr int IR_SENSITIVITY = 1;

    // AirGraffiti canvas.
    constexpr int OUTPUT_WIDTH = 1280;
    constexpr int OUTPUT_HEIGHT = 720;

    // Number of consecutive frames the IR source must remain
    // stable before a calibration point can be captured.
    constexpr int CALIBRATION_STABLE_FRAMES = 20;

    // Maximum movement between consecutive calibration frames
    // before the source is considered unstable.
    constexpr float CALIBRATION_STABILITY_PIXELS = 12.0f;

    // Drawing remains disabled until calibration succeeds.
    constexpr bool ENABLE_DRAWING = true;
}