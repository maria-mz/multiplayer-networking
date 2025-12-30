# Core game logic (shared by client and server)
add_library(game_core
    ${CMAKE_SOURCE_DIR}/game/Player.cpp
)
target_include_directories(game_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# Client executable
if (BUILD_CLIENT)
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
endif()

# Server executable
if (BUILD_SERVER)
  add_executable(server
      server.cpp
  )
  target_link_libraries(server PRIVATE
      game_core
  )
endif()
