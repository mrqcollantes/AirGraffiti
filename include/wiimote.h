#pragma once

struct wiimote_t;

// Represents one raw IR point detected by the Wii Remote.
struct WiiMoteIRPoint
{
    bool visible = false;

    // Raw Wiiuse coordinates.
    int rawX = 0;
    int rawY = 0;

    // Converted to AirGraffiti coordinates.
    float x = 0.0f;
    float y = 0.0f;
};

class WiiMote
{
    public:
        WiiMote();
        ~WiiMote();

        // Initialize the Wiiuse library and search for a Wii Remote.
        bool Init();
        void Update();
        void Shutdown();

        // Query the current state of the Wii Remote.
        bool IsInitialized() const;
        bool IsConnected() const;

        // True when at least one IR source is visible.
        bool HasIRPoint() const;

        // Number of visible IR sources (0-4).
        int GetVisiblePointCount() const;

        // Returns the primary IR point.
        const WiiMoteIRPoint& GetIRPoint() const;
    private:
        wiimote_t** wiimotes = nullptr;

        // Internal state
        bool initialized = false;
        bool connected = false;
        
        int visiblePointCount = 0;

        WiiMoteIRPoint irPoint;

        // Internal helpers
        void ResetIRData();
        void ProcessIRData();

        // Prevent copying
        WiiMote(const WiiMote&) = delete;
        WiiMote& operator=(const WiiMote&) = delete;
};