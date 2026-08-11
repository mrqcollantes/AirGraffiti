#include "canvas.h"
#include "renderer.h"

// The Canvas class manages an off-screen texture that can be drawn to and then rendered to the window.

Canvas::Canvas()
{
    texture = nullptr;
    width = 0;
    height = 0;
}

Canvas::~Canvas()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

bool Canvas::Create(Renderer& renderer, int width, int height)
{
    this->width = width;
    this->height = height;

    // Create an off-screen texture to serve as the canvas
    texture = SDL_CreateTexture(
        renderer.GetSDLRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height);

    if (!texture)
        return false;

    Clear(renderer);

    return true;
}

// Clears the canvas by filling it with a white background.
void Canvas::Clear(Renderer& renderer)
{
    SDL_SetRenderTarget(renderer.GetSDLRenderer(), texture);
    SDL_SetRenderDrawColor(renderer.GetSDLRenderer(), 255, 255, 255, 255);
    SDL_RenderClear(renderer.GetSDLRenderer());
    SDL_SetRenderTarget(renderer.GetSDLRenderer(), nullptr);
}

void Canvas::Draw(Renderer& renderer)
{
    SDL_Renderer* sdlRenderer = renderer.GetSDLRenderer();

    // Make sure we are rendering to the window
    SDL_SetRenderTarget(sdlRenderer, nullptr);

    SDL_RenderTexture(sdlRenderer, texture, nullptr, nullptr);
}

void Canvas::BeginDraw(Renderer& renderer)
{
    SDL_SetRenderTarget(renderer.GetSDLRenderer(), texture);
}

void Canvas::EndDraw(Renderer& renderer)
{
    SDL_SetRenderTarget(renderer.GetSDLRenderer(), nullptr);
}

void Canvas::DrawPoint(Renderer& renderer, int x, int y, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer.GetSDLRenderer(), color.r, color.g, color.b, color.a);
    SDL_RenderPoint(renderer.GetSDLRenderer(), static_cast<float>(x), static_cast<float>(y));
}

void Canvas::DrawLine(Renderer& renderer, int x1, int y1, int x2, int y2, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer.GetSDLRenderer(), color.r, color.g, color.b, color.a);
    SDL_RenderLine(renderer.GetSDLRenderer(),
        static_cast<float>(x1),
        static_cast<float>(y1),
        static_cast<float>(x2),
        static_cast<float>(y2)
    );
}