cmake_minimum_required (VERSION 3.8)

project(batnativeads)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wdeprecated -Wconditional-uninitialized -Wsign-conversion -Wshorten-64-to-32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")
set(CMAKE_CXX_STANDARD_REQUIRED, on)

include_directories(
  # BAT Native Ads
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src"

  # BAT Native User Model
  "${PROJECT_SOURCE_DIR}/deps/bat/usermodel/include"

  # RapidJSON
  "${PROJECT_SOURCE_DIR}/deps/bat/rapidjson/include"

  # URI Parser (Only required for mocks)
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/include")

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
  "${PROJECT_SOURCE_DIR}/src/mock_url_session.cc"
  "${PROJECT_SOURCE_DIR}/src/search_providers.cc"
  "${PROJECT_SOURCE_DIR}/src/string_helper.cc"
  "${PROJECT_SOURCE_DIR}/src/time_helper.cc"

set(USERMODEL_SOURCES
  "${PROJECT_SOURCE_DIR}/deps/bat/usermodel/src/bag_of_words_extractor.cc"
  "${PROJECT_SOURCE_DIR}/deps/bat/usermodel/src/naive_bayes.cc"
  "${PROJECT_SOURCE_DIR}/deps/bat/usermodel/src/stmr.c"
  "${PROJECT_SOURCE_DIR}/deps/bat/usermodel/src/user_model_impl.cc")

set(URIPARSER_SOURCES
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriCommon.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriCompare.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriEscape.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriFile.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriIp4.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriIp4Base.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriMemory.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriNormalize.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriNormalizeBase.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriParse.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriParseBase.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriQuery.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriRecompose.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriResolve.c"
  "${PROJECT_SOURCE_DIR}/third_party/uriparser/src/UriShorten.c")

add_executable(batnativeads
  "${PROJECT_SOURCE_DIR}/src/bat_native_ads.cc"
  "${PROJECT_SOURCE_DIR}/src/mock_ads_client.cc"
  ${SOURCES}
  ${USERMODEL_SOURCES}
  ${URIPARSER_SOURCES})
