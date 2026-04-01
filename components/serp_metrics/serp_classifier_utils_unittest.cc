/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier_utils.h"

#include "base/containers/fixed_flat_set.h"
#include "components/search_engines/search_engine_type.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace serp_metrics {

TEST(SerpClassifierUtilsTest, IsAllowedSearchEngine) {
  constexpr auto kExpectedSearchEngines =
      base::MakeFixedFlatSet<SearchEngineType>(
          base::sorted_unique,
          {SEARCH_ENGINE_BING, SEARCH_ENGINE_GOOGLE, SEARCH_ENGINE_YAHOO,
           SEARCH_ENGINE_DUCKDUCKGO, SEARCH_ENGINE_QWANT, SEARCH_ENGINE_ECOSIA,
           SEARCH_ENGINE_BRAVE, SEARCH_ENGINE_STARTPAGE});

  for (int i = 0; i <= SEARCH_ENGINE_MAX; ++i) {
    const SearchEngineType search_engine_type =
        static_cast<SearchEngineType>(i);
    EXPECT_EQ(IsAllowedSearchEngine(search_engine_type),
              kExpectedSearchEngines.contains(search_engine_type));
  }
}

// Web searches (no vertical params).
TEST(SerpClassifierUtilsTest, IsGoogleWebSearchWithNoSpecialParams) {
  EXPECT_TRUE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo)")));
}

TEST(SerpClassifierUtilsTest, IsGoogleWebSearchWithUdmZero) {
  EXPECT_TRUE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=0)")));
}

TEST(SerpClassifierUtilsTest, IsGoogleWebSearchWithUdmTwentyEight) {
  EXPECT_TRUE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=28)")));
}

// Vertical searches via tbm.
TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmImages) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=isch)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmNews) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=nws)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmVideo) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=vid)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmShopping) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=shop)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmBooks) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=bks)")));
}

// Vertical searches via udm.
TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithUdmImages) {
  EXPECT_FALSE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=2)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithUdmVideo) {
  EXPECT_FALSE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=7)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithUdmShopping) {
  EXPECT_FALSE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=14)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithUdmDiscussions) {
  EXPECT_FALSE(
      IsGoogleWebSearch(GURL(R"(https://www.google.com/search?q=foo&udm=18)")));
}

TEST(SerpClassifierUtilsTest, IsNotGoogleWebSearchWithTbmAndUdm) {
  EXPECT_FALSE(IsGoogleWebSearch(
      GURL(R"(https://www.google.com/search?q=foo&tbm=vid&udm=14)")));
}

}  // namespace serp_metrics
