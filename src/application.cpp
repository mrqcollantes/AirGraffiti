#include "application.h"
#include "wiimote_config.h"

#include <cstdio>

Application::Application()
    : wiimoteManager(WiiMoteConfig::DEFAULT_WIIMOTE_COUNT)
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
        std::printf(
            "[Application] Wii Remote initialization failed.\n");

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
    wiimoteManager.Update();

    // Mouse drawing remains unchanged.
    if (input.GetPointer().drawing)
    {
        int windowWidth = 0;
        int windowHeight = 0;

        renderer.GetWindowSize(windowWidth, windowHeight);

        float scaleX =
            static_cast<float>(canvas.GetWidth()) /
            static_cast<float>(windowWidth);

        float scaleY =
            static_cast<float>(canvas.GetHeight()) /
            static_cast<float>(windowHeight);

        brush.DrawStroke(
            canvas,
            renderer,
            static_cast<int>(input.GetPointer().previousX * scaleX),
            static_cast<int>(input.GetPointer().previousY * scaleY),
            static_cast<int>(input.GetPointer().x * scaleX),
            static_cast<int>(input.GetPointer().y * scaleY));
    }

    const FusedPoint& fused = wiimoteManager.GetFusedPoint();

    if (fused.valid)
    {
        irWasActive = true;

        int x = static_cast<int>(fused.x);
        int y = canvas.GetHeight() - static_cast<int>(fused.y);

        float displayWidth = 0.0f;
        float displayHeight = 0.0f;
        ui.GetDisplaySize(displayWidth, displayHeight);

        float windowX =
            static_cast<float>(x) *
            (displayWidth /
             static_cast<float>(canvas.GetWidth()));

        float windowY =
            static_cast<float>(y) *
            (displayHeight /
             static_cast<float>(canvas.GetHeight()));

        ui.SubmitPointerPosition(windowX, windowY);

        bool overUI =
            windowY >=
            (displayHeight - UI::PANEL_HEIGHT);

        if (overUI != wasOverUI)
        {
            ui.SubmitPointerButton(overUI);
            wasOverUI = overUI;
        }

        if (!overUI && previousIRValid)
        {
            brush.DrawStroke(
                canvas,
                renderer,
                previousIRX,
                previousIRY,
                x,
                y);
        }
        else if (!overUI)
        {
            brush.DrawStroke(
                canvas,
                renderer,
                x,
                y,
                x,
                y);
        }

        if (overUI)
        {
            previousIRValid = false;
        }
        else
        {
            previousIRX = x;
            previousIRY = y;
            previousIRValid = true;
        }
    }
    else
    {
        if (irWasActive)
        {
            ui.SubmitPointerButton(false);
            irWasActive = false;
        }

        wasOverUI = false;
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
    wiimoteManager.Shutdown();
    ui.Shutdown();
    renderer.Shutdown();
}
