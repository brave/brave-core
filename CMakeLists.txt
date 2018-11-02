cmake_minimum_required (VERSION 3.8)

project(batnativeads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wc++14-extensions -Wall -Wextra")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

include_directories(
  "include"
  "deps/bat/usermodel/include"
  "deps/bat/confirmations/include"
  "deps/bat/rapidjson/include"
  "third_party/uriparser/include")

file(GLOB SOURCES
  "src/*.cc"
  "deps/bat/usermodel/src/*.c*"
  "deps/bat/confirmations/src/*.cc"
  "third_party/uriparser/src/*.c")

add_executable(batnativeads ${SOURCES})
