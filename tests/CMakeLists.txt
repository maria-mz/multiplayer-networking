include(FetchContent)

if (BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
  )

  FetchContent_MakeAvailable(googletest)

  add_executable(tests
    network_server_test.cpp
    network_client_test.cpp
    udp_transport_test.cpp
    tcp_connection_test.cpp
    thread_safe_queue_test.cpp
  )

  target_link_libraries(tests PRIVATE game_core GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(tests)
endif()
