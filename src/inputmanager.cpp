#include "inputmanager.h"

InputManager::InputManager()
{
    quit = false;
}

void InputManager::Update()
{
    SDL_Event event;

    pointer.previousX = pointer.x;
    pointer.previousY = pointer.y;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            quit = true;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            pointer.x = event.motion.x;
            pointer.y = event.motion.y;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                pointer.drawing = true;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                pointer.drawing = false;
            }
            break;
        }
    }
}

bool InputManager::ShouldQuit() const
{
    return quit;
}

Pointer& InputManager::GetPointer()
{
    return pointer;
}