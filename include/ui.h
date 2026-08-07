#pragma once

#include <SDL3/SDL.h>

class Renderer;
class Brush;
class Canvas;

class UI
{
public:
    UI();

    bool Init(Renderer& renderer);
    void BeginFrame();
    void Draw(Brush& brush, Canvas& canvas, Renderer& renderer);
    void EndFrame();
    void Shutdown();

private:
    SDL_Renderer* sdlRenderer = nullptr;
};