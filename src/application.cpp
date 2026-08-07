#include "application.h"

Application::Application()
{
    running = true;
}

bool Application::Init()
{
    if (!renderer.Init("AirGraffiti", 1280, 720))
        return false;

    canvas.Create(renderer, 1280, 720);

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
    if (input.GetPointer().drawing)
    {
        brush.DrawStroke(
            canvas,
            renderer,
            static_cast<int>(input.GetPointer().previousX),
            static_cast<int>(input.GetPointer().previousY),
            static_cast<int>(input.GetPointer().x),
            static_cast<int>(input.GetPointer().y)
        );
    }
}

void Application::Render()
{
    renderer.BeginFrame();
    canvas.Draw(renderer);
    renderer.EndFrame();
}

void Application::Shutdown()
{
    renderer.Shutdown();
}