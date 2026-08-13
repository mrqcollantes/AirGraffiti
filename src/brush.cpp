#include "brush.h"
#include "canvas.h"
#include "renderer.h"

#include <cmath>

// The Brush class represents a simple drawing tool that can
// draw strokes on a Canvas using a specified color and size.
// In Eraser mode, the brush draws white to match the canvas background.

Brush::Brush()
{
    color = {0, 0, 0, 255}; // Black
    size = 16;
    mode = Mode::Spray;
}

void Brush::SetColor(SDL_Color newColor)
{
    color = newColor;
}

void Brush::SetSize(int newSize)
{
    if (newSize < 1)
        newSize = 1;

    size = newSize;
}

void Brush::SetMode(Mode newMode)
{
    mode = newMode;
}

SDL_Color Brush::GetColor() const
{
    return color;
}

int Brush::GetSize() const
{
    return size;
}

Brush::Mode Brush::GetMode() const
{
    return mode;
}

void Brush::DrawCircle(Canvas& canvas, Renderer& renderer, int x, int y)
{
    int radius = size / 2;

    // The canvas currently has a white background, so erasing is
    // implemented by drawing white over the existing artwork.
    SDL_Color drawColor = color;

    if (mode == Mode::Eraser)
    {
        drawColor = {255, 255, 255, 255};
    }

    for (int py = -radius; py <= radius; py++)
    {
        for (int px = -radius; px <= radius; px++)
        {
            if ((px * px) + (py * py) <= radius * radius)
            {
                canvas.DrawPoint(renderer, x + px, y + py, drawColor);
            }
        }
    }
}

void Brush::DrawStroke(Canvas& canvas, Renderer& renderer, int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    canvas.BeginDraw(renderer);

    if (distance < 1.0f)
    {
        DrawCircle(canvas, renderer, x1, y1);
    }
    else
    {
        float stepX = dx / distance;
        float stepY = dy / distance;
        float currentX = static_cast<float>(x1);
        float currentY = static_cast<float>(y1);

        for (int i = 0; i <= static_cast<int>(distance); i++)
        {
            DrawCircle(canvas, renderer, static_cast<int>(currentX), static_cast<int>(currentY));

            currentX += stepX;
            currentY += stepY;
        }
    }

    canvas.EndDraw(renderer);
}