# AirGraffiti

AirGraffiti is a C++ application that uses Wii Remotes and the [Wiiuse](https://github.com/wiiuse/wiiuse) library to track infrared (IR) emitters and translate their positions into a virtual pointer for an LED display. The system is designed to support multiple Wii Remotes for larger displays and flexible sensor placement.

## Architecture

The application is organized into several main components:

* **Wiiuse** — Handles communication with the Wii Remotes and provides IR tracking data.
* **Remote Management** — Maintains a dynamic collection of Wii Remotes and their individual connection, IR, and calibration states.
* **Calibration** — Each remote has its own calibration profile, allowing remotes to be positioned independently around the display.
* **Position Fusion** — Combines valid tracking data from 1 or more remotes into a unified pointer position.
* **UI** — Uses the resulting pointer position for drawing and interaction.
* **Renderer** — Handles rendering the AirGraffiti interface and graphics.

The default configuration uses **2 Wii Remotes**, but the architecture is designed to scale to additional remotes without requiring a redesign.

## Source Files

The `src/` directory contains the main application source code:

| File                     | Description                                                                            |
| ------------------------ | -------------------------------------------------------------------------------------- |
| `main.cpp`               | Application entry point; initializes and starts the program.                           |
| `application.cpp`        | Manages the main application lifecycle and program flow.                               |
| `renderer.cpp`           | Handles SDL rendering and display output.                                              |
| `ui.cpp`                 | Manages the user interface, drawing, and UI interaction.                               |
| `canvas.cpp`             | Manages the drawing canvas and its contents.                                           |
| `brush.cpp`              | Handles brush properties and drawing behavior.                                         |
| `pointer.cpp`            | Represents and manages the virtual pointer position and state.                         |
| `inputmanager.cpp`       | Processes keyboard and other application input.                                        |
| `wiimote.cpp`            | Represents an individual Wii Remote and its tracking state.                            |
| `wiimotemanager.cpp`     | Discovers, connects, and manages multiple Wii Remotes.                                 |
| `wiimotecalibration.cpp` | Handles per-remote IR calibration and coordinate mapping.                              |
| `irfusion.cpp`           | Combines valid IR tracking data from multiple remotes into a unified pointer position. |

## Running

### Option 1 — Run the existing build

From the project root:

```powershell
.\build\Debug\AirGraffiti.exe
```

### Option 2 — Build and run with CMake

In Visual Studio Code:

1. **Ctrl + shift + p**
2. **CMake: Configure**
3. **CMake: Build**
4. Run the **CMake: Debug** target.

The executable will be generated under:

```text
build/Debug/AirGraffiti.exe
```