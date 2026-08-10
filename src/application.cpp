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
    if (input.GetPointer().drawing)
    {
        int windowWidth = 0;
        int windowHeight = 0;
        renderer.GetWindowSize(windowWidth, windowHeight);

        float scaleX = static_cast<float>(canvas.GetWidth()) / static_cast<float>(windowWidth);
        float scaleY = static_cast<float>(canvas.GetHeight()) / static_cast<float>(windowHeight);

        brush.DrawStroke(
            canvas,
            renderer,
            static_cast<int>(input.GetPointer().previousX * scaleX),
            static_cast<int>(input.GetPointer().previousY * scaleY),
            static_cast<int>(input.GetPointer().x * scaleX),
            static_cast<int>(input.GetPointer().y * scaleY)
        );
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
    ui.Shutdown();
    renderer.Shutdown();
}