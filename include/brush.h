#pragma once

#include <SDL3/SDL.h>

class Canvas;
class Renderer;

class Brush
{
public:
    Brush();

    void DrawStroke(Canvas& canvas, Renderer& renderer, int x1, int y1, int x2, int y2);
    void SetColor(SDL_Color newColor);
    void SetSize(int newSize);

    SDL_Color GetColor() const;
    int GetSize() const;
private:
    SDL_Color color;
    int size;

    void DrawCircle(Canvas& canvas, Renderer& renderer, int x, int y);
};