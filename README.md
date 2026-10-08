# AirGraffiti

AirGraffiti is a C++ application that tracks infrared (IR) emitters with Raspberry Pi cameras and translates their positions into a virtual pointer for an LED display. A small Python detector runs on the Raspberry Pi, finds the IR spot in each camera frame, and streams its position over UDP to the AirGraffiti application on a Windows PC. The system is designed to support multiple cameras for larger displays and flexible sensor placement.

**SDL3** provides the foundation for the application, handling the window, rendering, and input needed to run AirGraffiti. **Dear ImGui** provides the graphical user interface, including the controls for brush size, colors, tools, and canvas interaction. It is integrated with SDL3 to display the interface within the application.

## Architecture

The system has two parts that communicate over the network:

```text
Raspberry Pi                                   Windows PC
┌──────────────────────────────┐               ┌──────────────────────────────┐
│ Pi camera                    │               │ AirGraffiti (C++)            │
│   └─ ir_detect.py            │ ── UDP ────>  │   ├─ Camera Manager          │
│       (OpenCV + Picamera2)   │   port 5005   │   ├─ Pointer                 │
└──────────────────────────────┘               │   ├─ UI / Canvas / Brush     │
                                               │   └─ Renderer                │
                                               └──────────────────────────────┘
```

* **IR Detector (`ir_detect.py`)** — Runs on the Raspberry Pi. Captures frames from the Pi camera, finds the brightest IR blob, and sends its position over UDP.
* **Camera Manager** — Receives the UDP packets on the PC and keeps track of each connected camera and its latest detection.
* **Pointer** — Turns the detected position into a virtual pointer position.
* **UI** — Uses the resulting pointer position for drawing and interaction.
* **Renderer** — Handles rendering the AirGraffiti interface and graphics.

Each camera is identified by a `--camera-id`, so multiple Raspberry Pis (or multiple cameras) can send to the same application.

## Source Files

The `src/` directory contains the main application source code:

| File                 | Description                                                      |
| -------------------- | ---------------------------------------------------------------- |
| `main.cpp`           | Application entry point; initializes and starts the program.     |
| `application.cpp`    | Manages the main application lifecycle and program flow.         |
| `renderer.cpp`       | Handles SDL rendering and display output.                        |
| `ui.cpp`             | Manages the user interface, drawing, and UI interaction.         |
| `canvas.cpp`         | Manages the drawing canvas and its contents.                     |
| `brush.cpp`          | Handles brush properties and drawing behavior.                   |
| `pointer.cpp`        | Represents and manages the virtual pointer position and state.   |
| `inputmanager.cpp`   | Processes keyboard and other application input.                  |
| `camera_manager.cpp` | Receives detection packets from the Raspberry Pi cameras via UDP. |

Camera-related settings live in `include/camera_config.h`.

The Raspberry Pi detector script is kept separately:

| File                   | Description                                                                 |
| ---------------------- | --------------------------------------------------------------------------- |
| `scripts/ir_detect.py` | Runs on the Raspberry Pi; detects the IR spot and sends it to the PC by UDP. |

## Raspberry Pi Setup

### Requirements

* A Raspberry Pi with a camera module (an IR-sensitive or NoIR camera works best)
* Raspberry Pi OS with `picamera2` and OpenCV installed:

```bash
sudo apt update
sudo apt install -y python3-picamera2 python3-opencv
```

### Running the detector

Copy `ir_detect.py` to the Pi, then run it with the IP address of the Windows PC running AirGraffiti:

```bash
python3 ir_detect.py --windows-ip 192.168.1.100
```

Useful options:

| Option           | Default         | Description                                                     |
| ---------------- | --------------- | --------------------------------------------------------------- |
| `--windows-ip`   | `192.168.1.100` | IP address of the PC running AirGraffiti.                       |
| `--udp-port`     | `5005`          | UDP port to send to (must match the application).               |
| `--camera-id`    | `0`             | ID of this camera; use a different value for each camera.       |
| `--threshold`    | `220`           | Minimum brightness (0-255) for a pixel to count as part of the spot. |
| `--min-area`     | `40`            | Smallest blob area (in pixels) to accept.                       |
| `--max-area`     | `10000`         | Largest blob area (in pixels) to accept.                        |
| `--exposure`     | `0`             | Manual exposure in microseconds (`0` = auto). Lower values help an IR LED stand out. |
| `--gain`         | `1.0`           | Camera gain (only used with `--exposure`).                      |
| `--width`/`--height` | `640` / `480` | Capture resolution.                                          |
| `--preview`      | off             | Shows the camera view and the detection mask (requires a display on the Pi). Press `Q` to quit. |

Example with two cameras and a short exposure:

```bash
# Pi #1
python3 ir_detect.py --windows-ip 192.168.1.100 --camera-id 0 --exposure 2000

# Pi #2
python3 ir_detect.py --windows-ip 192.168.1.100 --camera-id 1 --exposure 2000
```

### UDP packet format

Each frame, the detector sends one ASCII packet:

```text
camera_id,visible,x,y,area,level
```

| Field       | Description                                                          |
| ----------- | -------------------------------------------------------------------- |
| `camera_id` | ID of the camera that sent the packet.                               |
| `visible`   | `1` if an IR spot was found, `0` otherwise.                          |
| `x`, `y`    | Spot position in camera pixels (`0` when not visible).               |
| `area`      | Spot area in pixels (`0` when not visible).                          |
| `level`     | Brightness at the spot, 0-255 (`0` when not visible).                |

When the detector stops, it sends one final "not visible" packet so the application knows the IR source is gone.

### Tips

* Make sure the Pi and the PC are on the same network, and that the Windows firewall allows incoming UDP on port `5005`.
* Start with `--preview` to check what the detector sees, then tune `--threshold` and `--exposure` until only the IR emitter shows up as white in the mask window.
* A darker exposure usually works better than a high threshold for separating an IR LED from room lighting.

## Running AirGraffiti

Start the detector on the Raspberry Pi first (or at any time; the application simply waits for packets), then run the application on the PC.

### Option 1 — Run the existing build

From the project root:

```powershell
.\build\Debug\AirGraffiti.exe
```

### Option 2 — Build and run with CMake

In Visual Studio Code:

1. **Ctrl + Shift + P**
2. **CMake: Configure**
3. **CMake: Build**
4. Run the **CMake: Debug** target.

The executable will be generated under:

```text
build/Debug/AirGraffiti.exe
```