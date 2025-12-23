#include "../game/App.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::string serverIP = "127.0.0.1";
    int serverPort = 54000;

    try
    {
        App app(serverIP, serverPort);

        if (!app.start())
        {
            return 1;
        }
        app.run();
    }
    catch (const std::exception& e)
    {
        return 1;
    }

    return 0;
}
