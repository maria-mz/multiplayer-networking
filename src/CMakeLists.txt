# Core game logic (shared by client and server)
add_library(game_core
  ${CMAKE_SOURCE_DIR}/game/player.cpp
)
target_include_directories(game_core PUBLIC
  ${CMAKE_SOURCE_DIR}
)

# Client executable
if (BUILD_CLIENT)
  add_executable(client
      client/client.cpp
  )
  target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}
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
    server/server.cpp
  )
  target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}
  )
  target_link_libraries(server PRIVATE
    game_core
  )
endif()
