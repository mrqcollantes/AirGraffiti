#include "application.h"

int main(int argc, char* argv[])
{
    Application app;

    if (!app.Init())
        return -1;

    app.Run();
    app.Shutdown();
    return 0;
}