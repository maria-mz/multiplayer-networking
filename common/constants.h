#pragma once

#include <string>
#include <memory>

namespace constants
{
    // Window
    constexpr const char* WINDOW_TITLE = "Multiplayer Networking";
    constexpr int WINDOW_WIDTH = 720;
    constexpr int WINDOW_HEIGHT = 480;

    // Assets
    const std::string FILE_FONT_MAIN = std::string(ASSETS_DIR) + "/Inconsolata.ttf";

    // Networking
    enum class TransportType { TCP, UDP };
}
