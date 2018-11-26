set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

# Main
add_executable(main
  main.cc)

target_link_libraries(main
  gtest)

# Tabs Google Tests
add_executable(tabs
  test_tabs.cc
  ${SOURCES}
  ${USER_MODEL_SOURCES}
  ${URIPARSER_SOURCES})

target_link_libraries(tabs
  gtest
  gmock_main)

add_test(NAME tabs_test COMMAND tabs)
