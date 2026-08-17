#include "wiimotemanager.h"
#include "wiimote_config.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>

#include <wiiuse.h>

namespace
{
    void PrintCalibrationStatus(const WiiMoteManager& manager, bool allConnected)
    {
        static std::string lastStatus;
        std::string status;

        if (!allConnected)
        {
            status = "[WiiMoteCalibration] WAITING - Both TOP and LEFT remotes must be connected.";
        }
        else if (manager.GetSystemState() == WiiMoteSystemState::Ready)
        {
            status = "[WiiMoteCalibration] READY - Press A to restart calibration.";
        }
        else if (manager.GetSystemState() != WiiMoteSystemState::Calibrating)
        {
            status = "[WiiMoteCalibration] READY TO CALIBRATE - Press A, then point at TOP-LEFT.";
        }
        else
        {
            static const char* corners[] =
            {
                "TOP-LEFT",
                "TOP-RIGHT",
                "BOTTOM-RIGHT",
                "BOTTOM-LEFT"
            };

            const int step = manager.GetCalibrationStep();

            if (step >= 4)
                status = "[WiiMoteCalibration] CALCULATING 1D AXIS CALIBRATION...";
            else
                status =
                    std::string("[WiiMoteCalibration] CALIBRATION - Both remotes must see the source at ") +
                    corners[step] + ", hold steady, then press A.";
        }

        if (status != lastStatus)
        {
            std::printf("\n%s\n", status.c_str());
            lastStatus = status;
        }
    }

    // Peak-to-peak spread of a window: how far the readings wandered instead of how far last two frames differed.
    float WindowSpread(const std::deque<float>& window)
    {
        const auto minMax = std::minmax_element(window.begin(), window.end());
        return *minMax.second - *minMax.first;
    }

    float WindowAverage(const std::deque<float>& window)
    {
        const float sum = std::accumulate(window.begin(), window.end(), 0.0f);
        return sum / static_cast<float>(window.size());
    }

    // Live per-remote diagnostic, throttled and refreshed in place (\r) so
    // the user can watch it update in real time while repositioning the
    // remotes, without needing to press A to find out which one dropped.
    void PrintLiveIRStatus(const WiiMoteManager& manager)
    {
        static int throttle = 0;
        if (++throttle % 6 != 0)
            return;

        const auto& top = manager.GetRemoteState(WiiMoteManager::TOP_REMOTE);
        const auto& left = manager.GetRemoteState(WiiMoteManager::LEFT_REMOTE);

        char topField[48];
        char leftField[48];

        if (!top.connected)
            std::snprintf(topField, sizeof(topField), "DISCONNECTED");
        else if (top.irActive)
            std::snprintf(topField, sizeof(topField), "SEEN  (dots=%d, x=%.0f)", top.visiblePointCount, top.rawIR.x);
        else
            std::snprintf(topField, sizeof(topField), "NOT SEEN");

        if (!left.connected)
            std::snprintf(leftField, sizeof(leftField), "DISCONNECTED");
        else if (left.irActive)
            std::snprintf(leftField, sizeof(leftField), "SEEN  (dots=%d, x=%.0f)", left.visiblePointCount, left.rawIR.x);
        else
            std::snprintf(leftField, sizeof(leftField), "NOT SEEN");

        std::printf("\r[WiiMoteCalibration] TOP: %-24s | LEFT: %-24s   ", topField, leftField);
        std::fflush(stdout);
    }
}

WiiMoteManager::WiiMoteManager(std::size_t count)
    : remoteCount(REMOTE_COUNT)
{
    // This tracking method is specifically a two-remote system: remote 0 = TOP and remote 1 = LEFT.
    (void)count;

    states.resize(remoteCount);
    calibrationSamples.resize(4);
}

WiiMoteManager::~WiiMoteManager()
{
    Shutdown();
}

bool WiiMoteManager::Init()
{
    if (initialized)
        return true;

    std::printf("[WiiMoteManager] Initializing two-remotes axis tracking (R0=TOP, R1=LEFT)...\n");

    wiimotes = wiiuse_init(WiiMoteConfig::MAX_WIIMOTES);
    if (!wiimotes)
    {
        std::printf("[WiiMoteManager] ERROR: wiiuse_init() failed.\n");
        return false;
    }

    initialized = true;
    return ConnectRemotes();
}

bool WiiMoteManager::ConnectRemotes()
{
    ResetStates();

    std::printf("[WiiMoteManager] Searching for TOP and LEFT Wii Remotes...\n");

    const int found = wiiuse_find(wiimotes, static_cast<int>(REMOTE_COUNT), 5);

    if (found <= 0)
    {
        std::printf("[WiiMoteManager] No Wii Remotes found.\n");
        return true;
    }

    const int connected = wiiuse_connect(wiimotes, found);

    if (connected <= 0)
    {
        std::printf("[WiiMoteManager] Could not connect to any Wii Remote.\n");
        return true;
    }

    for (int i = 0; i < connected && i < static_cast<int>(REMOTE_COUNT); ++i)
    {
        if (!wiimotes[i])
            continue;

        states[i].connected = true;
        ConfigureRemote(wiimotes[i]);

        std::printf("[WiiMoteManager] Remote %d connected (%s).\n",
            i, i == static_cast<int>(TOP_REMOTE) ? "TOP" : "LEFT");
    }

    if (!AllRemotesConnected())
        std::printf("[WiiMoteManager] WARNING: Both remotes are required. R0=TOP, R1=LEFT.\n");

    return true;
}

void WiiMoteManager::ConfigureRemote(wiimote_t* remote)
{
    if (!remote)
        return;

    wiiuse_set_ir(remote, 1);
    wiiuse_set_ir_vres(remote, WiiMoteConfig::OUTPUT_WIDTH, WiiMoteConfig::OUTPUT_HEIGHT);
    wiiuse_set_ir_sensitivity(remote, WiiMoteConfig::IR_SENSITIVITY);
    wiiuse_set_aspect_ratio(remote, WIIUSE_ASPECT_16_9);
}

void WiiMoteManager::Update()
{
    if (!initialized || !wiimotes)
        return;

    if (!wiiuse_poll(wiimotes, static_cast<int>(REMOTE_COUNT)))
        return;

    bool aPressed = false;

    for (std::size_t i = 0; i < REMOTE_COUNT; ++i)
    {
        if (wiimotes[i] &&
            WIIMOTE_IS_CONNECTED(wiimotes[i]) &&
            wiimotes[i]->event == WIIUSE_EVENT &&
            IS_JUST_PRESSED(wiimotes[i], WIIMOTE_BUTTON_A))
        {
            aPressed = true;
        }

        if (!wiimotes[i])
        {
            states[i] = {};
            continue;
        }

        ProcessRemote(i, wiimotes[i]);
    }

    if (!AllRemotesConnected())
    {
        systemState = WiiMoteSystemState::WaitingForRemotes;
        fusedPoint = {};
        ClearCalibrationProgress();
        PrintCalibrationStatus(*this, false);
        return;
    }

    if (aPressed)
    {
        if (systemState == WiiMoteSystemState::WaitingForRemotes || systemState == WiiMoteSystemState::Ready)
        {
            ResetCalibration();

            std::printf(
                "\n==================================================\n"
                "       TWO-REMOTE CALIBRATION STARTED\n"
                "==================================================\n"
                "R0 TOP  -> screen X\n"
                "R1 LEFT -> screen Y\n"
                "\n"
                "Both remotes MUST see the same IR source.\n"
                "Point at TOP-LEFT, hold steady, then press A.\n"
                "==================================================\n");
        }
        else if (systemState == WiiMoteSystemState::Calibrating)
        {
            if (TryCaptureCalibrationPoint())
            {
                if (calibrationStep < 4)
                {
                    static const char* nextPoint[] =
                    {
                        "TOP-LEFT",
                        "TOP-RIGHT",
                        "BOTTOM-RIGHT",
                        "BOTTOM-LEFT"
                    };

                    std::printf(
                        "\n[WiiMoteCalibration] Captured point %d/4.\n"
                        "Next: %s.\n"
                        "Both remotes must see the source. "
                        "Hold steady, then press A.\n",
                        calibrationStep, nextPoint[calibrationStep]);
                }
            }
            else
            {
                std::printf(
                    "\n[WiiMoteCalibration] CANNOT CAPTURE.\n"
                    "Both remotes must see the same source and remain stable.\n"
                    "Stable frames: %d/%d\n",
                    stableFrames, WiiMoteConfig::CALIBRATION_STABLE_FRAMES);
            }
        }
    }

    if (!calibration.IsCalibrated())
    {
        UpdateCalibrationStability();
        fusedPoint = {};
        PrintCalibrationStatus(*this, true);

        if (systemState == WiiMoteSystemState::Calibrating)
            PrintLiveIRStatus(*this);

        return;
    }

    systemState = WiiMoteSystemState::Ready;
    UpdateTrackedPoint();

    static int debugFrameCounter = 0;
    if (++debugFrameCounter % 6 != 0)
        return;

    const auto& top = states[TOP_REMOTE];
    const auto& left = states[LEFT_REMOTE];

    char topField[24];
    char leftField[24];
    char screenField[32];

    if (top.irActive)
        std::snprintf(topField, sizeof(topField), "TOP X=%d", static_cast<int>(top.rawIR.x));
    else
        std::snprintf(topField, sizeof(topField), "TOP NO IR");

    if (left.irActive)
        std::snprintf(leftField, sizeof(leftField), "LEFT X=%d", static_cast<int>(left.rawIR.x));
    else
        std::snprintf(leftField, sizeof(leftField), "LEFT NO IR");

    if (fusedPoint.valid)
        std::snprintf(screenField, sizeof(screenField), "(%d,%d)",
            static_cast<int>(fusedPoint.x), static_cast<int>(fusedPoint.y));
    else
        std::snprintf(screenField, sizeof(screenField), "NONE");

    std::printf("\r[WiiMote] %s | %s | Screen=%s                    ", topField, leftField, screenField);
    std::fflush(stdout);
}

void WiiMoteManager::ProcessRemote(std::size_t index, wiimote_t* remote)
{
    if (index >= states.size())
        return;

    auto& state = states[index];

    if (remote->event == WIIUSE_DISCONNECT || remote->event == WIIUSE_UNEXPECTED_DISCONNECT)
    {
        state = {};
        std::printf("[WiiMoteManager] Remote %zu (%s) disconnected.\n",
            index, index == TOP_REMOTE ? "TOP" : "LEFT");
        return;
    }

    state.connected = true;
    state.irActive = false;
    state.visiblePointCount = 0;
    state.rawIR = {};

    for (int dotIndex = 0; dotIndex < 4; ++dotIndex)
    {
        if (remote->ir.dot[dotIndex].visible)
            ++state.visiblePointCount;
    }

    // One visible IR source is sufficient. We intentionally use the first
    // visible dot because this system is tracking one source, not a 4-point
    // Wii pointing pose.
    if (state.visiblePointCount <= 0)
        return;

    for (int dotIndex = 0; dotIndex < 4; ++dotIndex)
    {
        if (!remote->ir.dot[dotIndex].visible)
            continue;

        const ir_dot_t& dot = remote->ir.dot[dotIndex];

        state.rawIR.visible = true;
        state.rawIR.rawX = dot.rx;
        state.rawIR.rawY = dot.ry;
        state.rawIR.x = static_cast<float>(dot.rx);
        state.rawIR.y = static_cast<float>(dot.ry);
        state.irActive = true;
        return;
    }
}

bool WiiMoteManager::AllRemotesConnected() const
{
    return states.size() >= REMOTE_COUNT && states[TOP_REMOTE].connected && states[LEFT_REMOTE].connected;
}

bool WiiMoteManager::AllRemotesSeeingIR() const
{
    return AllRemotesConnected() &&
           states[TOP_REMOTE].irActive &&
           states[LEFT_REMOTE].irActive &&
           states[TOP_REMOTE].rawIR.visible &&
           states[LEFT_REMOTE].rawIR.visible;
}

bool WiiMoteManager::IsReady() const
{
    return systemState == WiiMoteSystemState::Ready;
}

WiiMoteSystemState WiiMoteManager::GetSystemState() const
{
    return systemState;
}

int WiiMoteManager::GetCalibrationStep() const
{
    return calibrationStep;
}

bool WiiMoteManager::CaptureCalibrationPoint()
{
    return TryCaptureCalibrationPoint();
}

bool WiiMoteManager::TryCaptureCalibrationPoint()
{
    if (!initialized || systemState != WiiMoteSystemState::Calibrating || calibrationStep >= 4)
        return false;

    if (!AllRemotesSeeingIR())
        return false;

    if (stableFrames < WiiMoteConfig::CALIBRATION_STABLE_FRAMES)
        return false;

    if (calibrationSamples.size() != 4)
        calibrationSamples.resize(4);

    // Use the averaged, already-stable window rather than the single
    // instantaneous frame that happened to cross the threshold - the user
    // held steady to earn this window, so use all of it.
    WiiMoteCalibrationSample sample;
    sample.topX = WindowAverage(topXWindow);
    sample.leftX = WindowAverage(leftXWindow);

    calibrationSamples[calibrationStep] = sample;

    ++calibrationStep;
    ClearCalibrationProgress();

    std::printf("\n[WiiMoteCalibration] Captured synchronized point %d/4 (TOP X=%.1f, LEFT X=%.1f).\n",
        calibrationStep, sample.topX, sample.leftX);

    if (calibrationStep < 4)
        return true;

    if (!calibration.Calculate(calibrationSamples,
            static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH),
            static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT)))
    {
        std::printf("[WiiMoteCalibration] Failed to calculate axis mappings. Restart calibration with A.\n");
        ResetCalibration();
        return false;
    }

    systemState = WiiMoteSystemState::Ready;

    const auto& topCalibration = calibration.GetTopCalibration();
    const auto& leftCalibration = calibration.GetLeftCalibration();

    std::printf(
        "\n==================================================\n"
        "       TWO-REMOTE CALIBRATION COMPLETE\n"
        "==================================================\n"
        "TOP  X -> screen X: scale=%f offset=%f\n"
        "LEFT X -> screen Y: scale=%f offset=%f\n"
        "Both remotes are required during runtime tracking.\n"
        "==================================================\n",
        topCalibration.scale, topCalibration.offset,
        leftCalibration.scale, leftCalibration.offset);

    return true;
}

void WiiMoteManager::UpdateCalibrationStability()
{
    if (!AllRemotesSeeingIR())
    {
        // A single dropped dot (camera flicker, momentary occlusion) is
        // normal and shouldn't discard an otherwise-good hold - give it
        // the same grace period as position jitter below.
        if (++unstableStreak > MAX_GRACE_FRAMES)
        {
            stableFrames = 0;
            unstableStreak = 0;
            topXWindow.clear();
            leftXWindow.clear();
        }
        return;
    }

    topXWindow.push_back(states[TOP_REMOTE].rawIR.x);
    leftXWindow.push_back(states[LEFT_REMOTE].rawIR.x);

    if (topXWindow.size() > STABILITY_WINDOW)
        topXWindow.pop_front();

    if (leftXWindow.size() > STABILITY_WINDOW)
        leftXWindow.pop_front();

    const bool windowFull = topXWindow.size() == STABILITY_WINDOW && leftXWindow.size() == STABILITY_WINDOW;

    if (!windowFull)
    {
        // Still filling the window for the first time; not enough history yet to judge stability either way.
        stableFrames = 0;
        return;
    }

    const float stabilityLimit = WiiMoteConfig::CALIBRATION_STABILITY_PIXELS;

    const bool stable =
        WindowSpread(topXWindow) <= stabilityLimit && WindowSpread(leftXWindow) <= stabilityLimit;

    if (stable)
    {
        ++stableFrames;
        unstableStreak = 0;
    }
    else if (++unstableStreak <= MAX_GRACE_FRAMES)
    {
        // Brief glitch (a flickered IR dot, a reflection) - don't throw
        // away an otherwise-good hold over one bad frame.
    }
    else
    {
        stableFrames = 0;
        unstableStreak = 0;
    }
}

void WiiMoteManager::UpdateTrackedPoint()
{
    // Runtime tracking deliberately requires both remotes.
    if (!AllRemotesSeeingIR())
    {
        fusedPoint = {};
        return;
    }

    fusedPoint = calibration.Apply(
        states[TOP_REMOTE].rawIR.x,
        states[LEFT_REMOTE].rawIR.x);
}

void WiiMoteManager::ResetCalibration()
{
    calibration.Reset();
    calibrationStep = 0;
    ClearCalibrationProgress();
    calibrationSamples.assign(4, {});
    fusedPoint = {};

    systemState = (initialized && AllRemotesConnected())
        ? WiiMoteSystemState::Calibrating
        : WiiMoteSystemState::WaitingForRemotes;
}

std::size_t WiiMoteManager::GetRemoteCount() const
{
    return remoteCount;
}

std::size_t WiiMoteManager::GetConnectedCount() const
{
    std::size_t count = 0;

    for (const auto& state : states)
    {
        if (state.connected)
            ++count;
    }

    return count;
}

bool WiiMoteManager::IsInitialized() const
{
    return initialized;
}

const WiiMoteManager::RemoteState&
WiiMoteManager::GetRemoteState(std::size_t index) const
{
    static const RemoteState emptyState{};

    if (index >= states.size())
        return emptyState;

    return states[index];
}

const std::vector<WiiMoteManager::RemoteState>&
WiiMoteManager::GetRemoteStates() const
{
    return states;
}

const FusedPoint& WiiMoteManager::GetFusedPoint() const
{
    return fusedPoint;
}

const WiiMoteCalibration& WiiMoteManager::GetCalibration() const
{
    return calibration;
}

void WiiMoteManager::ResetStates()
{
    states.assign(remoteCount, {});
    calibration.Reset();
    calibrationSamples.assign(4, {});
    calibrationStep = 0;
    ClearCalibrationProgress();
    fusedPoint = {};
    systemState = WiiMoteSystemState::WaitingForRemotes;
}

// Shared by every reset path (disconnect, restart, manual reset) so the
// stability window and its counters can never drift out of sync with
// each other.
void WiiMoteManager::ClearCalibrationProgress()
{
    stableFrames = 0;
    unstableStreak = 0;
    topXWindow.clear();
    leftXWindow.clear();
}

void WiiMoteManager::Shutdown()
{
    if (!initialized && !wiimotes)
        return;

    if (wiimotes)
    {
        for (std::size_t i = 0; i < REMOTE_COUNT; ++i)
        {
            if (wiimotes[i])
                wiiuse_disconnect(wiimotes[i]);
        }

        wiiuse_cleanup(wiimotes, static_cast<int>(WiiMoteConfig::MAX_WIIMOTES));
        wiimotes = nullptr;
    }

    initialized = false;
    ResetStates();

    std::printf("[WiiMoteManager] Shutdown complete.\n");
}