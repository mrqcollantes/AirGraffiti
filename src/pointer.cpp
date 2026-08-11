#include "pointer.h"

// The Pointer class represents the state of a pointing device (e.g., mouse or WiiMote).

Pointer::Pointer()
{
    x = 0.0f;
    y = 0.0f;

    previousX = 0.0f;
    previousY = 0.0f;

    drawing = false;
}