#pragma once

namespace CameraConfig
{
    // Raspberry Pi camera resolution.
    constexpr int CAMERA_WIDTH = 640;
    constexpr int CAMERA_HEIGHT = 480;

    // AirGraffiti canvas.
    constexpr int OUTPUT_WIDTH = 1280;
    constexpr int OUTPUT_HEIGHT = 720;

    // UDP settings.
    constexpr int UDP_PORT = 5005;

    // Camera ID for the first camera.
    constexpr int CAMERA_ID = 0;

    // How long Windows will consider the IR point valid
    // after the last packet arrives.
    constexpr int IR_TIMEOUT_MS = 150;
}