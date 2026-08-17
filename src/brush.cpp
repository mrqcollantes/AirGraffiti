#include "brush.h"
#include "canvas.h"
#include "renderer.h"

#include <cmath>
#include <random>

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

    if (mode == Mode::Eraser)
    {
        // Eraser stays a solid, reliable disk - a "spray" eraser with
        // random gaps would leave partially-erased pixels behind.
        SDL_Color eraseColor = {255, 255, 255, 255};

        for (int py = -radius; py <= radius; py++)
        {
            for (int px = -radius; px <= radius; px++)
            {
                if ((px * px) + (py * py) <= radius * radius)
                {
                    canvas.DrawPoint(renderer, x + px, y + py, eraseColor);
                }
            }
        }
        return;
    }

    // Spray mode: instead of a uniform filled disk, scatter dots with
    // density that fades from center to edge (a cone, not a hard
    // circle), plus a little per-dot alpha jitter for grain. This is
    // what actually reads as "spray can" rather than "marker."
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    for (int py = -radius; py <= radius; py++)
    {
        for (int px = -radius; px <= radius; px++)
        {
            float dist = std::sqrt(static_cast<float>(px * px + py * py));

            if (dist > static_cast<float>(radius))
                continue;

            // 0 at center, 1 at edge. Squaring biases coverage toward
            // the center, like a real spray cone.
            float t = dist / static_cast<float>(radius);
            float density = (1.0f - t) * (1.0f - t);

            // Randomly skip this pixel - less likely to be skipped near
            // the center, more likely near the edge. This is what
            // produces the grainy, scattered edge instead of a hard
            // cutoff.
            if (unit(rng) > density)
                continue;

            SDL_Color dabColor = color;
            float alphaJitter = 0.6f + 0.4f * unit(rng); // 60%-100% of base alpha, for texture
            dabColor.a = static_cast<Uint8>(
                static_cast<float>(color.a) * density * alphaJitter);

            if (dabColor.a == 0)
                continue;

            canvas.DrawPoint(renderer, x + px, y + py, dabColor);
        }
    }
}

void Brush::DrawStroke(Canvas& canvas, Renderer& renderer, int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    canvas.BeginDraw(renderer);

    // Spray dabs use partial alpha for grain/falloff - make sure the
    // renderer actually blends it instead of overwriting outright.
    // Eraser always draws at full alpha, so this doesn't change its look.
    SDL_SetRenderDrawBlendMode(renderer.GetSDLRenderer(), SDL_BLENDMODE_BLEND);

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