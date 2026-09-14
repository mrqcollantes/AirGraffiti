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

    std::size_t GetRemoteCount() const;
    std::size_t GetConnectedCount() const;

    struct RemoteState
    {
        bool connected = false;
        bool irActive = false;
        int visiblePointCount = 0;
        WiiMoteIRPoint rawIR;

        // Frames since a dot was actually reported for this remote. 0 means
        // "seen this exact poll." Two remotes streaming continuous IR over
        // one Bluetooth radio are commonly serviced on alternating polls
        // rather than both in the same tick, so this bridges that gap
        // instead of treating every missed poll as "source lost."
        // See WiiMoteManager::IR_STALE_GRACE_FRAMES.
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

    // Stability is judged over a rolling window rather than a single
    // frame-to-frame delta, so a shaky hand doesn't stall calibration.
    // A sample is captured using the *average* of the window, once the
    // window has been stable for long enough.
    static constexpr std::size_t STABILITY_WINDOW = 15;

    // A single occluded/glitched IR frame during an otherwise-stable hold doesn't reset progress.
    static constexpr int MAX_GRACE_FRAMES = 2;

    std::deque<float> topXWindow;
    std::deque<float> leftXWindow;
    int stableFrames = 0;
    int unstableStreak = 0;

    // After a corner is captured, calibration waits for the source to
    // move away by more than jitter before it will start accumulating a
    // STABLE window for the next corner - otherwise a hand that's still
    // sitting on the just-captured point would immediately "capture" it
    // again next frame.
    bool awaitingMovement = false;
    float lastCapturedTopX = 0.0f;
    float lastCapturedLeftX = 0.0f;

    // Two remotes streaming continuous IR over one Bluetooth radio are
    // commonly serviced by the host on alternating polls rather than both
    // in the same tick - each remote's dot genuinely disappears from that
    // remote's report every other poll or so even though the physical IR
    // source never moved or was occluded. A remote's last-known-good IR
    // reading is held as "still active" for up to this many consecutive
    // missed polls before it's treated as truly lost. Tune upward if two
    // remotes still don't both read at once on a particular host/radio.
    static constexpr int IR_STALE_GRACE_FRAMES = 4;

    WiiMoteManager(const WiiMoteManager&) = delete;
    WiiMoteManager& operator=(const WiiMoteManager&) = delete;
};