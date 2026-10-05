#include "camera_manager.h"
#include "camera_config.h"

#include <cstdio>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#endif

namespace
{
    std::uint64_t GetTimeMs()
    {
        using namespace std::chrono;

        return static_cast<std::uint64_t>(
            duration_cast<milliseconds>(
                steady_clock::now().time_since_epoch()
            ).count()
        );
    }
}

CameraManager::CameraManager()
{
    // Default calibration maps the entire camera image
    // to the entire AirGraffiti canvas.

    scaleX =
        static_cast<float>(CameraConfig::OUTPUT_WIDTH) /
        static_cast<float>(CameraConfig::CAMERA_WIDTH);

    scaleY =
        static_cast<float>(CameraConfig::OUTPUT_HEIGHT) /
        static_cast<float>(CameraConfig::CAMERA_HEIGHT);
}

CameraManager::~CameraManager()
{
    Shutdown();
}

bool CameraManager::Init()
{
#ifdef _WIN32

    WSADATA wsaData{};

    const int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsaData
    );

    if (result != 0)
    {
        std::printf(
            "[CameraManager] WSAStartup failed: %d\n",
            result
        );

        return false;
    }

    SOCKET socketValue = socket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP
    );

    if (socketValue == INVALID_SOCKET)
    {
        std::printf(
            "[CameraManager] Failed to create UDP socket.\n"
        );

        WSACleanup();
        return false;
    }

    // Non-blocking socket so the render loop never waits
    // for the Raspberry Pi.

    u_long nonBlocking = 1;

    if (ioctlsocket(
            socketValue,
            FIONBIO,
            &nonBlocking) != 0)
    {
        std::printf(
            "[CameraManager] Failed to make socket non-blocking.\n"
        );

        closesocket(socketValue);
        WSACleanup();

        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(
        CameraConfig::UDP_PORT
    );

    if (bind(
            socketValue,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR)
    {
        std::printf(
            "[CameraManager] Failed to bind UDP port %d.\n",
            CameraConfig::UDP_PORT
        );

        closesocket(socketValue);
        WSACleanup();

        return false;
    }

    socketHandle =
        reinterpret_cast<void*>(socketValue);

    initialized = true;

    std::printf(
        "[CameraManager] Listening for camera %d on UDP port %d.\n",
        CameraConfig::CAMERA_ID,
        CameraConfig::UDP_PORT
    );

    return true;

#else

    std::printf(
        "[CameraManager] UDP implementation currently targets Windows.\n"
    );

    return false;

#endif
}

void CameraManager::Update()
{
    if (!initialized)
        return;

#ifdef _WIN32

    SOCKET socketValue =
        reinterpret_cast<SOCKET>(socketHandle);

    char buffer[256];

    sockaddr_in sender{};
    int senderLength = sizeof(sender);

    while (true)
    {
        const int bytesReceived = recvfrom(
            socketValue,
            buffer,
            sizeof(buffer) - 1,
            0,
            reinterpret_cast<sockaddr*>(&sender),
            &senderLength
        );

        if (bytesReceived == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();

            if (error == WSAEWOULDBLOCK)
                break;

            std::printf(
                "[CameraManager] recvfrom failed: %d\n",
                error
            );

            break;
        }

        if (bytesReceived <= 0)
            break;

        buffer[bytesReceived] = '\0';

        /*
            Expected packet:

                camera_id,visible,x,y,area,brightness

            Example:

                0,1,315,241,42.0,255

            Or when the IR source isn't visible:

                0,0,0,0,0,0
        */

        int cameraId = -1;
        int visible = 0;
        float rawX = 0.0f;
        float rawY = 0.0f;
        float area = 0.0f;
        float brightness = 0.0f;

        const int parsed = std::sscanf(
            buffer,
            "%d,%d,%f,%f,%f,%f",
            &cameraId,
            &visible,
            &rawX,
            &rawY,
            &area,
            &brightness
        );

        if (parsed != 6)
        {
            continue;
        }

        if (cameraId != CameraConfig::CAMERA_ID)
            continue;

        lastPacketTimeMs = GetTimeMs();

        if (!visible)
        {
            point.valid = false;
            continue;
        }

        point.valid = true;
        point.rawX = rawX;
        point.rawY = rawY;
        point.area = area;
        point.brightness = brightness;

        ApplyCalibration(rawX, rawY);
    }

    // If the Pi stops sending packets, don't leave the
    // last IR point active forever.

    if (point.valid)
    {
        const std::uint64_t now = GetTimeMs();

        if (now - lastPacketTimeMs >
            CameraConfig::IR_TIMEOUT_MS)
        {
            point.valid = false;
        }
    }

#endif
}

void CameraManager::ApplyCalibration(
    float rawX,
    float rawY)
{
    point.x = rawX * scaleX + offsetX;
    point.y = rawY * scaleY + offsetY;

    // Keep the point inside the canvas.

    if (point.x < 0.0f)
        point.x = 0.0f;

    if (point.y < 0.0f)
        point.y = 0.0f;

    if (point.x >
        static_cast<float>(CameraConfig::OUTPUT_WIDTH - 1))
    {
        point.x =
            static_cast<float>(CameraConfig::OUTPUT_WIDTH - 1);
    }

    if (point.y >
        static_cast<float>(CameraConfig::OUTPUT_HEIGHT - 1))
    {
        point.y =
            static_cast<float>(CameraConfig::OUTPUT_HEIGHT - 1);
    }
}

void CameraManager::Shutdown()
{
    if (!initialized)
        return;

#ifdef _WIN32

    if (socketHandle != nullptr)
    {
        SOCKET socketValue =
            reinterpret_cast<SOCKET>(socketHandle);

        closesocket(socketValue);
        socketHandle = nullptr;
    }

    WSACleanup();

#endif

    initialized = false;
    point = CameraPoint{};
}

bool CameraManager::IsInitialized() const
{
    return initialized;
}

bool CameraManager::IsIRActive() const
{
    return point.valid;
}

const CameraPoint& CameraManager::GetPoint() const
{
    return point;
}