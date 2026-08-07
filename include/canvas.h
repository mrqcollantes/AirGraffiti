#pragma once

#include <SDL3/SDL.h>

class Renderer;

class Canvas
{
public:
    Canvas();
    ~Canvas();

    bool Create(Renderer& renderer, int width, int height);

    void Draw(Renderer& renderer);
    void Clear(Renderer& renderer);
    void BeginDraw(Renderer& renderer);
    void EndDraw(Renderer& renderer);
    void DrawPoint(Renderer& renderer,
                   int x,
                   int y,
                   SDL_Color color);
    void DrawLine(Renderer& renderer,
                  int x1,
                  int y1,
                  int x2,
                  int y2,
                  SDL_Color color);

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:

    SDL_Texture* texture;

    int width;
    int height;
};