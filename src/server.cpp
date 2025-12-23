// server_main.cpp
#include <iostream>
#include <thread>
#include <chrono>

#include "../game/GameServer.h"

int main()
{
    const int port = 54000;

    GameServer server(port);
    server.start();

    while (true)
    {
        server.pump();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}
