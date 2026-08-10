#include "renderer.h"

Renderer::Renderer()
{
    window = nullptr;
    renderer = nullptr;
    width = 0;
    height = 0;
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Init(const std::string& title, int width, int height)
{
    this->width = width;
    this->height = height;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL Init Error: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        title.c_str(),
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        SDL_Log("Window Error: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(
        window,
        nullptr
    );

    if (!renderer)
    {
        SDL_Log("Renderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
        return false;
    }

    return true;
}

void Renderer::Shutdown()
{
    if (!renderer && !window)
        return;

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

void Renderer::BeginFrame()
{
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
}

void Renderer::EndFrame()
{
    SDL_RenderPresent(renderer);
}

void Renderer::GetWindowSize(int& outWidth, int& outHeight)
{
    SDL_GetWindowSize(window, &outWidth, &outHeight);
}

void Renderer::DrawPoint(int x, int y, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderPoint(renderer, static_cast<float>(x), static_cast<float>(y));
}

void Renderer::DrawLine(int x1, int y1, int x2, int y2, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(renderer, static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(x2), static_cast<float>(y2));
}

SDL_Renderer* Renderer::GetSDLRenderer()
{
    return renderer;
}

SDL_Window* Renderer::GetSDLWindow()
{
    return window;
}