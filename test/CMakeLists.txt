cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

# Google Tests
add_executable(tests
  main.cc
  test_tabs.cc
  test_is_mobile.cc)

target_link_libraries(tests
  gtest
  gmock_main
  batnativeads)

add_test(NAME tests COMMAND tests)
