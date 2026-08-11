#include "wiimote.h"
#include "wiimote_config.h"
#include <wiiuse.h>
#include <algorithm>
#include <cstdio>

// The WiiMote class encapsulates the functionality of a Nintendo Wii Remote using the Wiiuse library.

WiiMote::WiiMote()
{
    initialized = false;
    connected = false;
    wiimotes = nullptr;
    visiblePointCount = 0;
    ResetIRData();
}

WiiMote::~WiiMote()
{
    Shutdown();
}

bool WiiMote::Init()
{
    if (initialized)
        return true;

    std::printf("[WiiMote] Initializing...\n");

    // Allocate Wiiuse structures
    wiimotes = wiiuse_init(
        WiiMoteConfig::MAX_WIIMOTES
    );

    if (!wiimotes)
    {
        std::printf("[WiiMote] ERROR: wiiuse_init() failed.\n");
        return false;
    }

    initialized = true;

    // Search for Wii Remotes
    std::printf("[WiiMote] Searching for Wii Remote...\n");

    int found = wiiuse_find(wiimotes, WiiMoteConfig::MAX_WIIMOTES, 5);

    if (found <= 0)
    {
        std::printf("[WiiMote] No Wii Remote found.\n");
        return true;
    }

    // Connect
    int connectedCount = wiiuse_connect(wiimotes, found);

    if (connectedCount <= 0)
    {
        std::printf("[WiiMote] Could not connect to Wii Remote.\n");
        return true;
    }

    connected = true;

    std::printf("[WiiMote] Connected.\n");

    // Configure IR camera
    for (int i = 0; i < connectedCount; ++i)
    {
        if (!wiimotes[i])
            continue;

        // Enable IR camera.
        wiiuse_set_ir(wiimotes[i], 1);

        // Set the IR virtual resolution.
        wiiuse_set_ir_vres(wiimotes[i], WiiMoteConfig::OUTPUT_WIDTH, WiiMoteConfig::OUTPUT_HEIGHT);

        // Set IR sensitivity.
        wiiuse_set_ir_sensitivity(wiimotes[i], WiiMoteConfig::IR_SENSITIVITY);

        // Use 16:9 output space.
        wiiuse_set_aspect_ratio(wiimotes[i], WIIUSE_ASPECT_16_9);
    }
    std::printf("[WiiMote] IR camera ready.\n");
    return true;
}

// Update the Wii Remote state. This should be called once per frame.
void WiiMote::Update()
{
    if (!initialized)
        return;
    if (!connected)
        return;
    if (!wiimotes)
        return;
    
    // Poll Wiiuse
    if (!wiiuse_poll( wiimotes, WiiMoteConfig::MAX_WIIMOTES))
    {
        return;
    }

    // Check connected Wii Remotes
    for (int i = 0; i < WiiMoteConfig::MAX_WIIMOTES; ++i)
    {
        if (!wiimotes[i])
            continue;

        wiimote_t* wm = wiimotes[i];

        // Connection status
        if (wm->event == WIIUSE_DISCONNECT || wm->event == WIIUSE_UNEXPECTED_DISCONNECT)
        {
            connected = false;
            std::printf("[WiiMote] Disconnected.\n");
            ResetIRData();
            continue;
        }
        ProcessIRData();
    }
}

// IR processing
void WiiMote::ProcessIRData()
{
    // Keep the previous detection state so we can avoid printing the same message every frame.
    static bool wasDetecting = false;

    ResetIRData();

    if (!wiimotes)
        return;

    wiimote_t* wm = wiimotes[0];

    if (!wm)
        return;

    // Count visible IR sources
    for (int i = 0; i < 4; ++i)
    {
        if (wm->ir.dot[i].visible)
        {
            ++visiblePointCount;
        }
    }

    // No IR source.
    if (visiblePointCount < WiiMoteConfig::MIN_VISIBLE_POINTS)
    {
        if (wasDetecting)
        {
            std::printf("[IR] Signal lost\n");
        }

        wasDetecting = false;
        return;
    }

    // Select first visible IR source
    for (int i = 0; i < 4; ++i)
    {
        if (!wm->ir.dot[i].visible)
            continue;

        const ir_dot_t& dot = wm->ir.dot[i];

        irPoint.visible = true;

        // Raw Wii Remote coordinates
        irPoint.rawX = dot.rx;
        irPoint.rawY = dot.ry;

        // Normalize coordinates
        float normalizedX = static_cast<float>(dot.rx) / static_cast<float>(WiiMoteConfig::IR_WIDTH - 1);
        float normalizedY = static_cast<float>(dot.ry) / static_cast<float>(WiiMoteConfig::IR_HEIGHT - 1);

        normalizedX = std::clamp(normalizedX, 0.0f, 1.0f);
        normalizedY = std::clamp(normalizedY, 0.0f, 1.0f);

        // Convert to AirGraffiti coordinates
        irPoint.x = normalizedX * static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH);
        irPoint.y = normalizedY * static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT);

        // Calibration
        irPoint.x = irPoint.x * WiiMoteConfig::SCALE_X + WiiMoteConfig::OFFSET_X;
        irPoint.y = irPoint.y * WiiMoteConfig::SCALE_Y + WiiMoteConfig::OFFSET_Y;

        // Keep coordinates inside canvas
        irPoint.x = std::clamp(irPoint.x, 0.0f, static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH - 1));
        irPoint.y = std::clamp(irPoint.y, 0.0f, static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT - 1));

        // Debug output
        std::printf(
            "\r[IR] X: %7.1f  Y: %7.1f  Raw: (%4d, %4d)  Points: %d   ",
            irPoint.x, irPoint.y, irPoint.rawX, irPoint.rawY, visiblePointCount);

        std::fflush(stdout);

        break;
    }
}

// Reset IR state
void WiiMote::ResetIRData()
{
    visiblePointCount = 0;
    irPoint.visible = false;
    irPoint.rawX = 0;
    irPoint.rawY = 0;
    irPoint.x = 0.0f;
    irPoint.y = 0.0f;
}

// Shutdown
void WiiMote::Shutdown()
{
    if (!initialized)
        return;

    std::printf("[WiiMote] Shutting down...\n");

    if (wiimotes)
    {
        if (connected)
        {
            for (int i = 0; i < WiiMoteConfig::MAX_WIIMOTES; ++i)
            {
                if (wiimotes[i])
                {
                    wiiuse_disconnect(wiimotes[i]);
                }
            }
        }
        wiiuse_cleanup(wiimotes, WiiMoteConfig::MAX_WIIMOTES);
        wiimotes = nullptr;
    }

    connected = false;
    initialized = false;
    ResetIRData();
    std::printf("[WiiMote] Shutdown complete.\n");
}

// Status
bool WiiMote::IsInitialized() const
{
    return initialized;
}

bool WiiMote::IsConnected() const
{
    return connected;
}

bool WiiMote::HasIRPoint() const
{
    return visiblePointCount >= WiiMoteConfig::MIN_VISIBLE_POINTS;
}

int WiiMote::GetVisiblePointCount() const
{
    return visiblePointCount;
}

const WiiMoteIRPoint& WiiMote::GetIRPoint() const
{
    return irPoint;
}