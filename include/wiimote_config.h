#pragma once

// ============================================================
// AirGraffiti - Wii Remote Configuration
//
// This is the ONLY file intended for values that may need to
// change when testing the IR spray cans.
// ============================================================

namespace WiiMoteConfig
{
    constexpr int MAX_WIIMOTES = 1;

    // Wiiuse raw IR coordinates are: X = 0 - 1023, Y = 0 - 767
    constexpr int IR_WIDTH = 1024;
    constexpr int IR_HEIGHT = 768;

    // Wiiuse IR sensitivity. Valid range: 1 - 5
    constexpr int IR_SENSITIVITY = 1;

    constexpr int OUTPUT_WIDTH = 1280;
    constexpr int OUTPUT_HEIGHT = 720;

    // Minimum number of visible IR sources required to consider the Wii Remote "pointing" at the canvas.
    constexpr int MIN_VISIBLE_POINTS = 1;

    // Coordinate calibration, leave these alone until actual spray can test.
    constexpr float OFFSET_X = 0.0f;
    constexpr float OFFSET_Y = 0.0f;
    constexpr float SCALE_X = 1.0f;
    constexpr float SCALE_Y = 1.0f;

    // Placeholder.
    constexpr int SIGNAL_THRESHOLD = 0;

    // Keep this FALSE while testing the IR camera.
    constexpr bool ENABLE_DRAWING = false;
}