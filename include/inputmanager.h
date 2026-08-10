#pragma once

#include <SDL3/SDL.h>
#include "pointer.h"

class InputManager
{
    public:
        InputManager();
        void Update();
        bool ShouldQuit() const;
        Pointer& GetPointer();
    private:
        bool quit;
        Pointer pointer;
};