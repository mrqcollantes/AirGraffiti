#include "application.h"

// The main function serves as the entry point for the AirGraffiti application.
// It creates an instance of the Application class, initializes it, and enters the main loop.
// Upon exiting the loop, it shuts down the application and returns an exit code.

int main(int argc, char* argv[])
{
    Application app;

    if (!app.Init())
        return -1;

    app.Run();
    app.Shutdown();
    return 0;
}