#include "application.h"
#include "wiimote_config.h"

#include <cstdio>

Application::Application() : wiimoteManager(WiiMoteManager::REMOTE_COUNT)
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

    if (!wiimoteManager.Init())
        std::printf("[Application] Wii Remote initialization failed.\n");

    return true;
}

void Application::Run()
{
    while (running)
    {
        input.Update();

        if (input.ShouldQuit())
            running = false;

        Update();
        Render();
    }
}

void Application::Update()
{
    // Mouse input
    Pointer& mouse = input.GetPointer();

    float displayWidth = 0.0f;
    float displayHeight = 0.0f;
    ui.GetDisplaySize(displayWidth, displayHeight);

    if (mouse.drawing &&
        displayWidth > 0.0f &&
        displayHeight > 0.0f)
    {
        // Mouse coordinates are already in window coordinates.
        const float mouseX = mouse.x;
        const float mouseY = mouse.y;

        // Do not draw inside the UI panel.
        const bool mouseOverUI = mouseY >= (displayHeight - UI::PANEL_HEIGHT);

        if (!mouseOverUI)
        {
            const float scaleX = static_cast<float>(canvas.GetWidth()) / displayWidth;
            const float scaleY = static_cast<float>(canvas.GetHeight()) / displayHeight;

            brush.DrawStroke(canvas, renderer,
                static_cast<int>(mouse.previousX * scaleX),
                static_cast<int>(mouse.previousY * scaleY),
                static_cast<int>(mouseX * scaleX),
                static_cast<int>(mouseY * scaleY));
        }
    }

    // Wii Remote / IR input
    wiimoteManager.Update();

    const FusedPoint& fused =
        wiimoteManager.GetFusedPoint();

    // IR lost
    if (!fused.valid || !wiimoteManager.IsIRActiveForDrawing())
    {
        // If IR was controlling the UI, release the ImGui button.
        if (irWasActive && isUIPress)
        {
            ui.SubmitPointerButton(false);
        }

        // IR is no longer active.
        irWasActive = false;
        isUIPress = false;

        // The next IR position is a NEW stroke.
        previousIRValid = false;

        return;
    }

    // IR position
    const int x = static_cast<int>(fused.x);
    const int y = static_cast<int>(fused.y);

    // Convert canvas coordinates to ImGui/window coordinates.
    if (canvas.GetWidth() <= 0 || canvas.GetHeight() <= 0 || displayWidth <= 0.0f || displayHeight <= 0.0f)
    {
        return;
    }

    const float windowX = static_cast<float>(x) * (displayWidth / static_cast<float>(canvas.GetWidth()));
    const float windowY = static_cast<float>(y) * (displayHeight / static_cast<float>(canvas.GetHeight()));

    // Determine whether IR is over the UI
    const bool overUI =
        windowY >= (displayHeight - UI::PANEL_HEIGHT);

    // New IR interaction
    if (!irWasActive)
    {
        previousIRValid = false;
        isUIPress = overUI;
    }

    // IR controls UI
    if (isUIPress)
    {
        ui.SubmitPointerPosition(windowX, windowY);
        ui.SubmitPointerButton(true);

        // We aren't drawing a canvas stroke while interacting with the UI.
        previousIRValid = false;
    }
    // IR controls canvas
    else
    {
        if (previousIRValid)
        {
            brush.DrawStroke(canvas, renderer, previousIRX, previousIRY, x, y);
        }
        else
        {
            // First frame of a new IR interaction.
            brush.DrawStroke(canvas, renderer, x, y, x, y);
        }

        previousIRX = x;
        previousIRY = y;
        previousIRValid = true;
    }
    irWasActive = true;
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
    wiimoteManager.Shutdown();
    ui.Shutdown();
    renderer.Shutdown();
}