#include "application.h"
#include <cstdio>

// The Application class serves as the main entry point for the AirGraffiti application.
// It manages the initialization, main loop, and shutdown of the application, coordinating
// between the Renderer, Canvas, Brush, InputManager, UI, and WiiMote classes to provide a
// cohesive drawing experience.

Application::Application()
{
    running = true;
}

bool Application::Init()
{
    if (!renderer.Init("AirGraffiti", 1280, 720))
        return false;

    if (!canvas.Create(renderer, 1280, 720))
        return false;

    if (!ui.Init(renderer))
        return false;

    if (!wiimote.Init())
    {
        std::printf("[Application] Wii Remote initialization failed.\n");
    }

    return true;
}

void Application::Run()
{
    while (running)
    {
        input.Update();

        if (input.ShouldQuit())
        {
            running = false;
        }

        Update();
        Render();
    }
}

void Application::Update()
{
    wiimote.Update();

    // Mouse drawing
    if (input.GetPointer().drawing)
    {
        int windowWidth = 0;
        int windowHeight = 0;

        renderer.GetWindowSize(windowWidth, windowHeight);

        float scaleX = static_cast<float>(canvas.GetWidth()) / static_cast<float>(windowWidth);
        float scaleY = static_cast<float>(canvas.GetHeight()) / static_cast<float>(windowHeight);

        brush.DrawStroke(canvas, renderer,
            static_cast<int>(input.GetPointer().previousX * scaleX),
            static_cast<int>(input.GetPointer().previousY * scaleY),
            static_cast<int>(input.GetPointer().x * scaleX),
            static_cast<int>(input.GetPointer().y * scaleY)
        );
    }

    // Wii Remote IR drawing.
    static bool irWasActive = false;
    static bool wasOverUI = false;

    if (wiimote.HasIRPoint())
    {
        irWasActive = true;

        const WiiMoteIRPoint& ir = wiimote.GetIRPoint();

        // Convert Wiiuse IR coordinates to canvas coordinates
        int x = static_cast<int>(ir.x);
        int y = canvas.GetHeight() - static_cast<int>(ir.y);

        // Convert canvas coordinates to window coordinates for ImGui
        float displayWidth = 0.0f;
        float displayHeight = 0.0f;
        ui.GetDisplaySize(displayWidth, displayHeight);

        float windowX = static_cast<float>(x) *
            (displayWidth / static_cast<float>(canvas.GetWidth()));
        float windowY = static_cast<float>(y) *
            (displayHeight / static_cast<float>(canvas.GetHeight()));

        // Feed the synthetic pointer position into ImGui.
        ui.SubmitPointerPosition(windowX, windowY);

        // Feed a synthetic left mouse button event into ImGui only on real press/release edges.
        bool overUI = windowY >= (displayHeight - UI::PANEL_HEIGHT);

        // Feed a synthetic left mouse button event into ImGui only on real press/release edges.
        if (overUI != wasOverUI)
        {
            ui.SubmitPointerButton(overUI);
            wasOverUI = overUI;
        }

        // Debug output for IR point and UI overlap state
        static int debugLastX = 0;
        static int debugLastY = 0;
        static bool debugLastOverUI = false;
        static bool debugHasPrinted = false;

        if (!debugHasPrinted || x != debugLastX || y != debugLastY || overUI != debugLastOverUI)
        {
            std::printf("\r[IR->Draw] canvasX: %4d  canvasY: %4d  overUI: %s   ",
                x, y, overUI ? "YES" : "no ");
            std::fflush(stdout);

            debugLastX = x;
            debugLastY = y;
            debugLastOverUI = overUI;
            debugHasPrinted = true;
        }

        // Draw a stroke from the previous IR point to the current one.
        static bool previousIRValid = false;
        static int previousIRX = 0;
        static int previousIRY = 0;

        if (overUI)
        {
            // Don't draw on the canvas if the IR point is over the UI panel.
            previousIRValid = false;
        }
        else if (previousIRValid)
        {
            brush.DrawStroke(canvas, renderer, previousIRX, previousIRY, x, y);
            previousIRX = x;
            previousIRY = y;
        }
        else
        {
            // First detected IR point - just draw a single point.
            brush.DrawStroke(canvas, renderer, x, y, x, y);
            previousIRX = x;
            previousIRY = y;
            previousIRValid = true;
        }
    }
    else
    {
        // No IR point detected - if it was active last frame, send a release event.
        if (irWasActive)
        {
            ui.SubmitPointerButton(false);
            irWasActive = false;
        }
        wasOverUI = false;

        static bool previousIRValid = false;
        previousIRValid = false;
    }
}

void Application::Render()
{
    renderer.BeginFrame();

    canvas.Draw(renderer);

    ui.BeginFrame();
    ui.Draw(brush, canvas, renderer);
    ui.EndFrame();

    renderer.EndFrame();
}

void Application::Shutdown()
{
    wiimote.Shutdown();
    ui.Shutdown();
    renderer.Shutdown();
}