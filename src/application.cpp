#include "application.h"

#include <cstdio>

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

    if (!cameraManager.Init())
    {
        std::printf(
            "[Application] Camera initialization failed.\n"
        );
    }

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
    // ---------------------------------------------------------
    // Mouse input
    // ---------------------------------------------------------

    Pointer& mouse = input.GetPointer();

    float displayWidth = 0.0f;
    float displayHeight = 0.0f;

    ui.GetDisplaySize(
        displayWidth,
        displayHeight
    );

    if (mouse.drawing &&
        displayWidth > 0.0f &&
        displayHeight > 0.0f)
    {
        // Mouse coordinates are already in window coordinates.
        const float mouseX = mouse.x;
        const float mouseY = mouse.y;

        // Do not draw inside the UI panel.
        const bool mouseOverUI =
            mouseY >= (displayHeight - UI::PANEL_HEIGHT);

        if (!mouseOverUI)
        {
            const float scaleX =
                static_cast<float>(canvas.GetWidth()) /
                displayWidth;

            const float scaleY =
                static_cast<float>(canvas.GetHeight()) /
                displayHeight;

            brush.DrawStroke(
                canvas,
                renderer,
                static_cast<int>(mouse.previousX * scaleX),
                static_cast<int>(mouse.previousY * scaleY),
                static_cast<int>(mouseX * scaleX),
                static_cast<int>(mouseY * scaleY)
            );
        }
    }

    // ---------------------------------------------------------
    // Raspberry Pi camera / IR input
    // ---------------------------------------------------------

    cameraManager.Update();

    const CameraPoint& cameraPoint =
        cameraManager.GetPoint();

    // IR lost.
    if (!cameraPoint.valid ||
        !cameraManager.IsIRActive())
    {
        // If IR was controlling the UI, release the ImGui button.
        if (irWasActive && isUIPress)
            ui.SubmitPointerButton(false);

        irWasActive = false;
        isUIPress = false;

        // The next IR position is a NEW stroke.
        previousIRValid = false;

        return;
    }

    // ---------------------------------------------------------
    // Camera position
    // ---------------------------------------------------------

    const int x =
        static_cast<int>(cameraPoint.x);

    const int y =
        static_cast<int>(cameraPoint.y);

    if (canvas.GetWidth() <= 0 ||
        canvas.GetHeight() <= 0 ||
        displayWidth <= 0.0f ||
        displayHeight <= 0.0f)
    {
        return;
    }

    // Convert canvas coordinates to ImGui/window coordinates.
    const float windowX =
        static_cast<float>(x) *
        (displayWidth /
         static_cast<float>(canvas.GetWidth()));

    const float windowY =
        static_cast<float>(y) *
        (displayHeight /
         static_cast<float>(canvas.GetHeight()));

    // Determine whether IR is over the UI.
    const bool overUI =
        windowY >=
        (displayHeight - UI::PANEL_HEIGHT);

    // ---------------------------------------------------------
    // New IR interaction
    // ---------------------------------------------------------

    if (!irWasActive)
    {
        previousIRValid = false;
        isUIPress = overUI;
    }

    // ---------------------------------------------------------
    // IR controls UI
    // ---------------------------------------------------------

    if (isUIPress)
    {
        ui.SubmitPointerPosition(
            windowX,
            windowY
        );

        ui.SubmitPointerButton(true);

        // We aren't drawing a canvas stroke while
        // interacting with the UI.
        previousIRValid = false;
    }

    // ---------------------------------------------------------
    // IR controls canvas
    // ---------------------------------------------------------

    else
    {
        if (previousIRValid)
        {
            brush.DrawStroke(
                canvas,
                renderer,
                previousIRX,
                previousIRY,
                x,
                y
            );
        }
        else
        {
            // First frame of a new IR interaction.
            brush.DrawStroke(
                canvas,
                renderer,
                x,
                y,
                x,
                y
            );
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
    cameraManager.Shutdown();

    ui.Shutdown();
    renderer.Shutdown();
}