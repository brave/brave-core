cmake_minimum_required (VERSION 3.8)

project(batnativeads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wc++17-extensions -Wunused-private-field")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

include_directories(include
  deps
  deps/bat-native-rapidjson/include
  deps/bat-native-usermodel/include
  third_party/uriparser/include)

file(GLOB SOURCES
  "src/*.cc"
  "deps/bat-native-usermodel/src/*.cc"
  "deps/bat-native-usermodel/src/*.c"
  "deps/bat-native-confirmations/src/*.cc"
  "third_party/uriparser/src/*.c")

add_executable(batnativeads ${SOURCES})
