cmake_minimum_required (VERSION 3.8)

project(batnativeads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wunused-const-variable -Wunused-variable -Wconditional-uninitialized -Wsign-conversion -Wshorten-64-to-32")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

include(FetchContent)

# BAT Native User Model
FetchContent_Declare(
  BATNativeUserModelProject
  GIT_REPOSITORY git@github.com:brave-intl/bat-native-usermodel.git
  GIT_TAG        master
)

FetchContent_GetProperties(BATNativeUserModelProject)
if(NOT batnativeusermodelproject_POPULATED)
  message(STATUS "Fetching BAT Native User Model dependency")
  FetchContent_Populate(BATNativeUserModelProject)
endif()

# RapidJSON
FetchContent_Declare(
  RapidJSONProject
  GIT_REPOSITORY git@github.com:brave-intl/bat-native-rapidjson.git
  GIT_TAG        master
)

FetchContent_GetProperties(RapidJSONProject)
if(NOT rapidjsonproject_POPULATED)
  message(STATUS "Fetching RapidJSON dependency")
  FetchContent_Populate(RapidJSONProject)
endif()

# URI Parser (Only required for isolated mocks)
FetchContent_Declare(
  URIParserProject
  GIT_REPOSITORY git@github.com:uriparser/uriparser.git
  GIT_TAG        master
)

FetchContent_GetProperties(URIParserProject)
if(NOT uriparserproject_POPULATED)
message(STATUS "Fetching URI Parser dependency")
  FetchContent_Populate(URIParserProject)
endif()

# BAT Native Ads
include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src"

  # BAT Native User Model
  "${batnativeusermodelproject_SOURCE_DIR}/include"

  # RapidJSON
  "${rapidjsonproject_SOURCE_DIR}/include"

  # URI Parser (Only required for isolated mocks in bat-native-ads.cc/h)
  "${uriparserproject_SOURCE_DIR}/include")

set(SOURCES
  "${PROJECT_SOURCE_DIR}/src/bat/ads/ad_info.cc"
  "${PROJECT_SOURCE_DIR}/src/bat/ads/client_info.cc"
  "${PROJECT_SOURCE_DIR}/src/bat/ads/notification_info.cc"
  "${PROJECT_SOURCE_DIR}/src/bat/ads/url_components.cc"
  "${PROJECT_SOURCE_DIR}/src/ads.cc"
  "${PROJECT_SOURCE_DIR}/src/ads_impl.cc"
  "${PROJECT_SOURCE_DIR}/src/ads_serve.cc"
  "${PROJECT_SOURCE_DIR}/src/bundle.cc"
  "${PROJECT_SOURCE_DIR}/src/bundle_state.cc"
  "${PROJECT_SOURCE_DIR}/src/catalog.cc"
  "${PROJECT_SOURCE_DIR}/src/catalog_state.cc"
  "${PROJECT_SOURCE_DIR}/src/client.cc"
  "${PROJECT_SOURCE_DIR}/src/client_state.cc"
  "${PROJECT_SOURCE_DIR}/src/json_helper.cc"
  "${PROJECT_SOURCE_DIR}/src/math_helper.cc"
  "${PROJECT_SOURCE_DIR}/src/locale_helper.cc"
  "${PROJECT_SOURCE_DIR}/src/search_providers.cc"
  "${PROJECT_SOURCE_DIR}/src/string_helper.cc"
  "${PROJECT_SOURCE_DIR}/src/time_helper.cc")

set(USER_MODEL_SOURCES
  "${batnativeusermodelproject_SOURCE_DIR}/src/bag_of_words_extractor.cc"
  "${batnativeusermodelproject_SOURCE_DIR}/src/naive_bayes.cc"
  "${batnativeusermodelproject_SOURCE_DIR}/src/stmr.c"
  "${batnativeusermodelproject_SOURCE_DIR}/src/user_model_impl.cc")

set(URIPARSER_SOURCES
  "${uriparserproject_SOURCE_DIR}/src/UriCommon.c"
  "${uriparserproject_SOURCE_DIR}/src/UriCompare.c"
  "${uriparserproject_SOURCE_DIR}/src/UriEscape.c"
  "${uriparserproject_SOURCE_DIR}/src/UriFile.c"
  "${uriparserproject_SOURCE_DIR}/src/UriIp4.c"
  "${uriparserproject_SOURCE_DIR}/src/UriIp4Base.c"
  "${uriparserproject_SOURCE_DIR}/src/UriMemory.c"
  "${uriparserproject_SOURCE_DIR}/src/UriNormalize.c"
  "${uriparserproject_SOURCE_DIR}/src/UriNormalizeBase.c"
  "${uriparserproject_SOURCE_DIR}/src/UriParse.c"
  "${uriparserproject_SOURCE_DIR}/src/UriParseBase.c"
  "${uriparserproject_SOURCE_DIR}/src/UriQuery.c"
  "${uriparserproject_SOURCE_DIR}/src/UriRecompose.c"
  "${uriparserproject_SOURCE_DIR}/src/UriResolve.c"
  "${uriparserproject_SOURCE_DIR}/src/UriShorten.c")

add_executable(batnativeads
  "${PROJECT_SOURCE_DIR}/src/bat_native_ads.cc"
  "${PROJECT_SOURCE_DIR}/src/mock_ads_client.cc"
  ${SOURCES}
  ${USER_MODEL_SOURCES}
  ${URIPARSER_SOURCES})

# Google Tests
enable_testing()
add_subdirectory(test)

configure_file(CMakeLists-GoogleTests.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(
  ${CMAKE_CURRENT_BINARY_DIR}/googletest-src
  ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
  EXCLUDE_FROM_ALL)
