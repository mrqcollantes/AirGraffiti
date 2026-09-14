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
    // Corner names shared by the calibration-progress and capture-event
    // printers below, so both stay in sync with WiiMoteCalibration's
    // capture order (0=TL, 1=TR, 2=BR, 3=BL).
    const char* const kCornerNames[] =
    {
        "TOP-LEFT",
        "TOP-RIGHT",
        "BOTTOM-RIGHT",
        "BOTTOM-LEFT"
    };

    // Only fires for the coarse, infrequent state changes (waiting for
    // remotes / ready / about to start). Corner-by-corner progress while
    // Calibrating is handled entirely by PrintCalibrationProgress below.
    void PrintSystemStatus(const WiiMoteManager& manager, bool allConnected)
    {
        static std::string lastStatus;
        std::string status;

        if (!allConnected)
            status = "[WiiMoteCalibration] WAITING - Both TOP and LEFT remotes must be connected.";
        else if (manager.GetSystemState() == WiiMoteSystemState::Ready)
            status = "[WiiMoteCalibration] READY - Press A to restart calibration.";
        else if (manager.GetSystemState() != WiiMoteSystemState::Calibrating)
            status = "[WiiMoteCalibration] Press A to begin calibration (starts at TOP-LEFT).";
        else
            return;

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

    // Single refreshing progress line for the corner currently being
    // captured. Two phases, both drawn on the same \r line:
    //   - accumulating: STABLE count climbing toward the capture threshold
    //   - awaiting movement: just captured this corner, waiting for the
    //     source to move away before the next corner can start accumulating
    void PrintCalibrationProgress(const WiiMoteManager& manager)
    {
        const int step = manager.GetCalibrationStep();
        if (step >= 4)
            return;

        const auto& top = manager.GetRemoteState(WiiMoteManager::TOP_REMOTE);
        const auto& left = manager.GetRemoteState(WiiMoteManager::LEFT_REMOTE);

        char topField[16];
        char leftField[16];

        // DISC / NO IR / SEEN are distinct states - collapsing them into a
        // single '--' makes "not connected" indistinguishable from
        // "connected but blind", which is exactly what's hard to diagnose.
        // A trailing '~' means this reading is being held over from a
        // recent poll (grace window active) rather than freshly reported
        // this exact tick - expected to flicker on/off during normal
        // two-remote interleaving, not a sign of trouble by itself.
        if (!top.connected)
            std::snprintf(topField, sizeof(topField), "T:DISC");
        else if (!top.irActive)
            std::snprintf(topField, sizeof(topField), "T:NO IR");
        else if (top.framesSinceSeen > 0)
            std::snprintf(topField, sizeof(topField), "T:%.0f~", top.rawIR.x);
        else
            std::snprintf(topField, sizeof(topField), "T:%.0f", top.rawIR.x);

        if (!left.connected)
            std::snprintf(leftField, sizeof(leftField), "L:DISC");
        else if (!left.irActive)
            std::snprintf(leftField, sizeof(leftField), "L:NO IR");
        else if (left.framesSinceSeen > 0)
            std::snprintf(leftField, sizeof(leftField), "L:%.0f~", left.rawIR.x);
        else
            std::snprintf(leftField, sizeof(leftField), "L:%.0f", left.rawIR.x);

        // Ticks every call regardless of whether anything else changed, so
        // an unmoving line still visibly updates - proof the loop is alive
        // rather than looking hung when both remotes report the same thing
        // frame after frame.
        static const char heartbeatChars[] = { '|', '/', '-', '\\' };
        static int heartbeatIndex = 0;
        const char heartbeat = heartbeatChars[heartbeatIndex++ % 4];

        if (manager.IsAwaitingMovement())
        {
            std::printf("\r[Calib] %c %-13s CAPTURED - move source to start next point | %-8s %-8s        ",
                heartbeat, kCornerNames[step - 1], topField, leftField);
        }
        else
        {
            std::printf("\r[Calib] %c %-13s STABLE %2d/%2d | %-8s %-8s        ",
                heartbeat, kCornerNames[step], manager.GetStableFrames(), WiiMoteConfig::CALIBRATION_STABLE_FRAMES,
                topField, leftField);
        }

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

    // Force every printf to appear immediately instead of sitting in a
    // buffer until something else happens to flush it. Several of the
    // informational prints below (banners, connect/disconnect messages,
    // CAPTURED lines) don't call fflush() themselves, so under a buffered
    // stdout - the default on Windows/MSVC whenever stdout isn't attached
    // to a real interactive console - they can sit invisible for an
    // arbitrarily long time. This must happen before any printf below.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

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

    std::printf(
        "[WIIUSE] found=%d connected=%d MAX=%d\n",
        found,
        connected,
        WiiMoteConfig::MAX_WIIMOTES
    );

    for (int i = 0; i < WiiMoteConfig::MAX_WIIMOTES; ++i)
    {
        std::printf(
            "[WIIUSE] slot %d = %p\n",
            i,
            static_cast<void*>(wiimotes[i])
        );
    }

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

    // Without this, the remote only sends a report when a button's state
    // changes - IR/accel data just rides along on whatever report happens
    // to fire, so wiiuse_poll() returns nothing most frames and everything
    // downstream (dots, stability accumulation, the debug line) only
    // appears to update around a button press. This makes it stream
    // continuously instead.
    wiiuse_set_flags(remote, WIIUSE_CONTINUOUS, 0);

    wiiuse_set_ir(remote, 1);
    wiiuse_set_ir_vres(remote, WiiMoteConfig::OUTPUT_WIDTH, WiiMoteConfig::OUTPUT_HEIGHT);
    wiiuse_set_ir_sensitivity(remote, WiiMoteConfig::IR_SENSITIVITY);
    wiiuse_set_aspect_ratio(remote, WIIUSE_ASPECT_16_9);
}

void WiiMoteManager::Update()
{
    if (!initialized || !wiimotes)
        return;

    const int pollResult = wiiuse_poll(wiimotes, static_cast<int>(REMOTE_COUNT));

    if (!pollResult)
        return;

    static int rawDebugCounter = 0;

    if (++rawDebugCounter % 10 == 0)
    {
        for (std::size_t i = 0; i < REMOTE_COUNT; ++i)
        {
            if (!wiimotes[i])
            {
                std::printf("\n[RAW] R%zu | NULL", i);
                continue;
            }

            std::printf(
                "\n[RAW] R%zu | connected=%d | event=%d | dots=%d",
                i,
                WIIMOTE_IS_CONNECTED(wiimotes[i]),
                wiimotes[i]->event,
                wiimotes[i]->ir.num_dots
            );

            for (int d = 0; d < 4; ++d)
            {
                if (wiimotes[i]->ir.dot[d].visible)
                {
                    std::printf(
                        " | dot%d=(x=%d,y=%d)",
                        d,
                        wiimotes[i]->ir.dot[d].rx,
                        wiimotes[i]->ir.dot[d].ry
                    );
                }
            }
        }

        std::printf("\n");
        std::fflush(stdout);
    }

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
        PrintSystemStatus(*this, false);
        return;
    }

    if (aPressed && (systemState == WiiMoteSystemState::WaitingForRemotes || systemState == WiiMoteSystemState::Ready))
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
            "Point at TOP-LEFT and hold steady - capture is automatic.\n"
            "==================================================\n");
    }

    if (!calibration.IsCalibrated())
    {
        UpdateCalibrationStability();
        fusedPoint = {};

        if (systemState == WiiMoteSystemState::Calibrating)
        {
            if (TryCaptureCalibrationPoint())
            {
                const auto& sample = calibrationSamples[calibrationStep - 1];

                std::printf("\n[Calib] CAPTURED %d/4 (%-12s) TOP=%.1f LEFT=%.1f\n",
                    calibrationStep, kCornerNames[calibrationStep - 1], sample.topX, sample.leftX);

                if (calibrationStep < 4)
                    std::printf("[Calib] Move source to %s and hold steady.\n", kCornerNames[calibrationStep]);
            }

            PrintCalibrationProgress(*this);
        }
        else
        {
            PrintSystemStatus(*this, true);
        }

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

    if (top.irActive && top.framesSinceSeen > 0)
        std::snprintf(topField, sizeof(topField), "TOP X=%d~", static_cast<int>(top.rawIR.x));
    else if (top.irActive)
        std::snprintf(topField, sizeof(topField), "TOP X=%d", static_cast<int>(top.rawIR.x));
    else
        std::snprintf(topField, sizeof(topField), "TOP NO IR");

    if (left.irActive && left.framesSinceSeen > 0)
        std::snprintf(leftField, sizeof(leftField), "LEFT X=%d~", static_cast<int>(left.rawIR.x));
    else if (left.irActive)
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

    int visibleThisPoll = 0;
    for (int dotIndex = 0; dotIndex < 4; ++dotIndex)
    {
        if (remote->ir.dot[dotIndex].visible)
            ++visibleThisPoll;
    }

    // visiblePointCount always reflects THIS poll's literal truth, even
    // during a grace-held frame below - it's a diagnostic field, not part
    // of tracking logic, so it should never lie about what was actually
    // reported.
    state.visiblePointCount = visibleThisPoll;

    if (visibleThisPoll > 0)
    {
        // One visible IR source is sufficient. We intentionally use the
        // first visible dot because this system is tracking one source,
        // not a 4-point Wii pointing pose.
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
            break;
        }

        state.irActive = true;
        state.framesSinceSeen = 0;
        return;
    }

    // No dot in THIS poll. With two remotes streaming continuous IR over
    // the same Bluetooth radio, the host commonly services them on
    // alternating ticks rather than both in the same tick - R0 gets a
    // report this instant, R1 the next, back and forth. That's radio/
    // scheduling contention, not the source leaving view, so a short run
    // of misses is held over using the last-known-good reading instead of
    // being treated as "lost" immediately.
    ++state.framesSinceSeen;

    if (state.framesSinceSeen > IR_STALE_GRACE_FRAMES)
    {
        // Genuinely gone (occluded, out of range, etc.) - only now clear it.
        state.irActive = false;
        state.rawIR = {};
    }
    // else: irActive and rawIR are left exactly as they were on the last
    // poll the dot was actually seen on - a deliberate hold, not a bug.
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

int WiiMoteManager::GetStableFrames() const
{
    return stableFrames;
}

bool WiiMoteManager::IsAwaitingMovement() const
{
    return awaitingMovement;
}

bool WiiMoteManager::CaptureCalibrationPoint()
{
    return TryCaptureCalibrationPoint();
}

bool WiiMoteManager::TryCaptureCalibrationPoint()
{
    if (!initialized || systemState != WiiMoteSystemState::Calibrating || calibrationStep >= 4)
        return false;

    // Just captured this corner - don't fire again until the source has
    // moved away and a fresh STABLE window has been earned at the next one.
    if (awaitingMovement)
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

    const bool calibrationFinished = calibrationStep >= 4;

    ClearCalibrationProgress();

    if (!calibrationFinished)
    {
        awaitingMovement = true;
        lastCapturedTopX = sample.topX;
        lastCapturedLeftX = sample.leftX;
        return true;
    }

    if (!calibration.Calculate(calibrationSamples,
            static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH),
            static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT)))
    {
        std::printf("[Calib] Failed to calculate axis mappings. Press A to restart calibration.\n");
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
    if (awaitingMovement)
    {
        if (!AllRemotesSeeingIR())
            return;

        // Require a deliberate move, not just jitter, before treating the
        // source as "on the next corner" - a few stability-pixels' worth
        // isn't enough to distinguish a hand tremor from an actual move.
        const float moveThreshold = WiiMoteConfig::CALIBRATION_STABILITY_PIXELS * 4.0f;

        const float topDelta = std::fabs(states[TOP_REMOTE].rawIR.x - lastCapturedTopX);
        const float leftDelta = std::fabs(states[LEFT_REMOTE].rawIR.x - lastCapturedLeftX);

        if (topDelta < moveThreshold && leftDelta < moveThreshold)
            return;

        awaitingMovement = false;
        // Falls through to start accumulating the next corner's window below.
    }

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
    awaitingMovement = false;
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