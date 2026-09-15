#pragma once

#include <cstddef>
#include <deque>
#include <vector>

#include "wiimote_config.h"
#include "wiimotecalibration.h"

struct wiimote_t;

enum class WiiMoteSystemState
{
    WaitingForRemotes, Calibrating, Ready, Error
};

// Remote 0 = TOP (Remote X to Screen X), Remote 1 = LEFT (Remote X to Screen Y)
class WiiMoteManager
{
public:
    static constexpr std::size_t REMOTE_COUNT = 2;
    static constexpr std::size_t TOP_REMOTE = 0;
    static constexpr std::size_t LEFT_REMOTE = 1;

    explicit WiiMoteManager(std::size_t remoteCount = REMOTE_COUNT);
    ~WiiMoteManager();

    bool Init();
    void Update();
    void Shutdown();

    bool IsInitialized() const;
    bool IsReady() const;
    bool AllRemotesConnected() const;
    bool AllRemotesSeeingIR() const;
    bool IsIRActiveForDrawing() const;

    std::size_t GetRemoteCount() const;
    std::size_t GetConnectedCount() const;

    struct RemoteState
    {
        bool connected = false;
        bool irActive = false;
        int visiblePointCount = 0;
        WiiMoteIRPoint rawIR;
        int framesSinceSeen = 0;
    };

    const RemoteState& GetRemoteState(std::size_t index) const;
    const std::vector<RemoteState>& GetRemoteStates() const;

    const FusedPoint& GetFusedPoint() const;

    WiiMoteSystemState GetSystemState() const;
    int GetCalibrationStep() const;
    int GetStableFrames() const;
    bool IsAwaitingMovement() const;
    bool CaptureCalibrationPoint();
    void ResetCalibration();

    const WiiMoteCalibration& GetCalibration() const;

private:
    bool ConnectRemotes();
    void ConfigureRemote(wiimote_t* remote);
    void ProcessRemote(std::size_t index, wiimote_t* remote);
    bool TryCaptureCalibrationPoint();
    void UpdateCalibrationStability();
    void UpdateTrackedPoint();
    void ResetStates();
    void ClearCalibrationProgress();

    std::size_t remoteCount = REMOTE_COUNT;
    bool initialized = false;
    wiimote_t** wiimotes = nullptr;

    std::vector<RemoteState> states;
    WiiMoteCalibration calibration;
    FusedPoint fusedPoint;

    WiiMoteSystemState systemState = WiiMoteSystemState::WaitingForRemotes;

    // 0 = top-left, 1 = top-right, 2 = bottom-right, 3 = bottom-left.
    int calibrationStep = 0;
    std::vector<WiiMoteCalibrationSample> calibrationSamples;

    static constexpr std::size_t STABILITY_WINDOW = 15;
    static constexpr int MAX_GRACE_FRAMES = 2;

    std::deque<float> topXWindow;
    std::deque<float> leftXWindow;
    int stableFrames = 0;
    int unstableStreak = 0;

    bool awaitingMovement = false;
    float lastCapturedTopX = 0.0f;
    float lastCapturedLeftX = 0.0f;

    static constexpr int IR_STALE_GRACE_FRAMES = 4;

    WiiMoteManager(const WiiMoteManager&) = delete;
    WiiMoteManager& operator=(const WiiMoteManager&) = delete;
};