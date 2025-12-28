# Core game logic (shared by client and server)
add_library(game_core
    ${CMAKE_SOURCE_DIR}/game/Player.cpp
    ${CMAKE_SOURCE_DIR}/game/TextureManager.cpp
    ${CMAKE_SOURCE_DIR}/game/FontManager.cpp
)
target_include_directories(game_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# Client executable
add_executable(client
    client.cpp
)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)
target_link_libraries(client PRIVATE
    game_core
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
    SDL2_image::SDL2_image
)

# Server executable
add_executable(server
    server.cpp
)
target_link_libraries(server PRIVATE
    game_core
)
