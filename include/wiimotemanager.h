#pragma once

#include <cstddef>
#include <vector>

#include "wiimote_config.h"
#include "wiimotecalibration.h"
#include "irfusion.h"
#include "wiimote.h"

struct WiiMoteRemoteState
{
    bool connected = false;
    bool irActive = false;
    bool calibrated = false;

    int visiblePointCount = 0;

    WiiMoteIRPoint rawIR;
    WiiMoteIRPoint calibratedIR;
};

class WiiMoteManager
{
public:
    explicit WiiMoteManager(
        std::size_t remoteCount = WiiMoteConfig::DEFAULT_WIIMOTE_COUNT);
    ~WiiMoteManager();

    bool Init();
    void Update();
    void Shutdown();

    bool SetRemoteCount(std::size_t remoteCount);
    std::size_t GetRemoteCount() const;
    std::size_t GetConnectedCount() const;

    bool IsInitialized() const;

    const WiiMoteRemoteState& GetRemoteState(std::size_t remoteIndex) const;
    const std::vector<WiiMoteRemoteState>& GetRemoteStates() const;

    const FusedPoint& GetFusedPoint() const;

    WiiMoteCalibration& GetCalibration();
    const WiiMoteCalibration& GetCalibration() const;

private:
    bool ConnectRemotes();
    void ConfigureRemote(wiimote_t* remote);
    void ResetStates();
    void ProcessRemote(std::size_t index, wiimote_t* remote);

    std::size_t remoteCount;
    bool initialized = false;

    wiimote_t** wiimotes = nullptr;

    std::vector<WiiMoteRemoteState> states;
    WiiMoteCalibration calibration;
    AverageFusion fusion;
    FusedPoint fusedPoint;

    WiiMoteManager(const WiiMoteManager&) = delete;
    WiiMoteManager& operator=(const WiiMoteManager&) = delete;
};
