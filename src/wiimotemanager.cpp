#include "wiimotemanager.h"
#include "wiimote_config.h"

#include <algorithm>
#include <cstdio>

#include <wiiuse.h>

WiiMoteManager::WiiMoteManager(std::size_t count)
    : remoteCount(std::clamp<std::size_t>(
          count, 1, WiiMoteConfig::MAX_WIIMOTES)),
      calibration(remoteCount)
{
    states.resize(remoteCount);
}

WiiMoteManager::~WiiMoteManager()
{
    Shutdown();
}

bool WiiMoteManager::Init()
{
    if (initialized)
        return true;

    std::printf(
        "[WiiMoteManager] Initializing for %zu remote(s)...\n",
        remoteCount);

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

    std::printf(
        "[WiiMoteManager] Searching for up to %zu remote(s)...\n",
        remoteCount);

    const int found = wiiuse_find(
        wiimotes,
        static_cast<int>(remoteCount),
        5);

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

    for (int i = 0; i < connected && i < static_cast<int>(states.size()); ++i)
    {
        if (!wiimotes[i])
            continue;

        states[i].connected = true;
        ConfigureRemote(wiimotes[i]);

        std::printf(
            "[WiiMoteManager] Remote %d connected.\n",
            i);
    }

    return true;
}

void WiiMoteManager::ConfigureRemote(wiimote_t* remote)
{
    if (!remote)
        return;

    wiiuse_set_ir(remote, 1);
    wiiuse_set_ir_vres(
        remote,
        WiiMoteConfig::OUTPUT_WIDTH,
        WiiMoteConfig::OUTPUT_HEIGHT);
    wiiuse_set_ir_sensitivity(
        remote,
        WiiMoteConfig::IR_SENSITIVITY);
    wiiuse_set_aspect_ratio(
        remote,
        WIIUSE_ASPECT_16_9);
}

void WiiMoteManager::Update()
{
    if (!initialized || !wiimotes)
        return;

    if (!wiiuse_poll(
            wiimotes,
            static_cast<int>(remoteCount)))
    {
        return;
    }

    for (std::size_t i = 0; i < remoteCount; ++i)
    {
        if (!wiimotes[i])
        {
            states[i].connected = false;
            states[i].irActive = false;
            states[i].visiblePointCount = 0;
            states[i].rawIR = {};
            states[i].calibratedIR = {};
            continue;
        }

        ProcessRemote(i, wiimotes[i]);
    }

    std::vector<WiiMoteIRPoint> observations;
    observations.reserve(remoteCount);

    for (const auto& state : states)
    {
        if (state.connected && state.irActive)
            observations.push_back(state.calibratedIR);
    }

    fusedPoint = fusion.Fuse(observations);
}

void WiiMoteManager::ProcessRemote(
    std::size_t index,
    wiimote_t* remote)
{
    auto& state = states[index];

    if (remote->event == WIIUSE_DISCONNECT ||
        remote->event == WIIUSE_UNEXPECTED_DISCONNECT)
    {
        state.connected = false;
        state.irActive = false;
        state.visiblePointCount = 0;
        state.rawIR = {};
        state.calibratedIR = {};

        std::printf(
            "[WiiMoteManager] Remote %zu disconnected.\n",
            index);

        return;
    }

    state.connected = true;
    state.visiblePointCount = 0;
    state.rawIR = {};
    state.calibratedIR = {};
    state.irActive = false;

    // Preserve the existing behavior: use the first visible IR source
    // from each remote as that remote's primary pointing observation.
    for (int dotIndex = 0; dotIndex < 4; ++dotIndex)
    {
        if (!remote->ir.dot[dotIndex].visible)
            continue;

        ++state.visiblePointCount;
    }

    if (state.visiblePointCount < WiiMoteConfig::MIN_VISIBLE_POINTS)
        return;

    for (int dotIndex = 0; dotIndex < 4; ++dotIndex)
    {
        if (!remote->ir.dot[dotIndex].visible)
            continue;

        const ir_dot_t& dot = remote->ir.dot[dotIndex];

        WiiMoteIRPoint raw;
        raw.visible = true;
        raw.rawX = dot.rx;
        raw.rawY = dot.ry;

        float normalizedX =
            static_cast<float>(dot.rx) /
            static_cast<float>(WiiMoteConfig::IR_WIDTH - 1);

        float normalizedY =
            static_cast<float>(dot.ry) /
            static_cast<float>(WiiMoteConfig::IR_HEIGHT - 1);

        normalizedX = std::clamp(normalizedX, 0.0f, 1.0f);
        normalizedY = std::clamp(normalizedY, 0.0f, 1.0f);

        raw.x =
            normalizedX *
            static_cast<float>(WiiMoteConfig::OUTPUT_WIDTH);

        raw.y =
            normalizedY *
            static_cast<float>(WiiMoteConfig::OUTPUT_HEIGHT);

        state.rawIR = raw;
        state.calibratedIR = calibration.Apply(index, raw);
        state.irActive = true;
        state.calibrated = calibration.IsCalibrated(index);

        break;
    }
}

bool WiiMoteManager::SetRemoteCount(std::size_t count)
{
    count = std::clamp<std::size_t>(
        count,
        1,
        WiiMoteConfig::MAX_WIIMOTES);

    if (count == remoteCount)
        return true;

    const bool wasInitialized = initialized;

    Shutdown();

    remoteCount = count;
    states.assign(remoteCount, {});
    calibration.Resize(remoteCount);

    if (!wasInitialized)
        return true;

    return Init();
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

const WiiMoteRemoteState& WiiMoteManager::GetRemoteState(
    std::size_t remoteIndex) const
{
    static const WiiMoteRemoteState emptyState{};

    if (remoteIndex >= states.size())
        return emptyState;

    return states[remoteIndex];
}

const std::vector<WiiMoteRemoteState>&
WiiMoteManager::GetRemoteStates() const
{
    return states;
}

const FusedPoint& WiiMoteManager::GetFusedPoint() const
{
    return fusedPoint;
}

WiiMoteCalibration& WiiMoteManager::GetCalibration()
{
    return calibration;
}

const WiiMoteCalibration& WiiMoteManager::GetCalibration() const
{
    return calibration;
}

void WiiMoteManager::ResetStates()
{
    states.assign(remoteCount, {});
    calibration.Resize(remoteCount);
    fusedPoint = {};
}

void WiiMoteManager::Shutdown()
{
    if (!initialized && !wiimotes)
        return;

    if (wiimotes)
    {
        for (std::size_t i = 0; i < remoteCount; ++i)
        {
            if (wiimotes[i])
                wiiuse_disconnect(wiimotes[i]);
        }

        wiiuse_cleanup(
            wiimotes,
            static_cast<int>(WiiMoteConfig::MAX_WIIMOTES));

        wiimotes = nullptr;
    }

    initialized = false;
    ResetStates();

    std::printf("[WiiMoteManager] Shutdown complete.\n");
}