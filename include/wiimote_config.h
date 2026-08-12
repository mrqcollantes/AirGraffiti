#pragma once

namespace WiiMoteConfig
{
    // Maximum number of remotes the application can manage.
    constexpr int MAX_WIIMOTES = 5;

    // Number used by the application when no other count is selected.
    constexpr int DEFAULT_WIIMOTE_COUNT = 2;

    // Wiiuse raw IR coordinates.
    constexpr int IR_WIDTH = 1024;
    constexpr int IR_HEIGHT = 768;

    constexpr int IR_SENSITIVITY = 1;

    constexpr int OUTPUT_WIDTH = 1280;
    constexpr int OUTPUT_HEIGHT = 720;

    constexpr int MIN_VISIBLE_POINTS = 1;

    // Initial/default calibration. Individual profiles are owned by
    // WiiMoteCalibration and indexed by remote.
    constexpr float OFFSET_X = 0.0f;
    constexpr float OFFSET_Y = 0.0f;
    constexpr float SCALE_X = 1.0f;
    constexpr float SCALE_Y = 1.0f;

    constexpr int SIGNAL_THRESHOLD = 0;
    constexpr bool ENABLE_DRAWING = false;
}
