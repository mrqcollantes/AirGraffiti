#pragma once

#include <SDL3/SDL.h>

class Canvas;
class Renderer;

class Brush
{
public:
    enum class Mode
    {
        Spray,
        Eraser
    };

    Brush();

    void DrawStroke(Canvas& canvas, Renderer& renderer, int x1, int y1, int x2, int y2);

    void SetColor(SDL_Color newColor);
    void SetSize(int newSize);
    void SetMode(Mode newMode);

    SDL_Color GetColor() const;
    int GetSize() const;
    Mode GetMode() const;

private:
    SDL_Color color;
    int size;
    Mode mode;

    void DrawCircle(Canvas& canvas, Renderer& renderer, int x, int y);
};