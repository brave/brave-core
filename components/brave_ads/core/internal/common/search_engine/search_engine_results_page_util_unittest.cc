/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSearchEngineResultsPageUtilTest, IsSearchEngineResultsPage) {
  // Arrange

  // Act

  // Assert
  for (const auto& url : GetSearchEngineResultsPageUrls()) {
    EXPECT_TRUE(IsSearchEngineResultsPage(url));
  }
}

TEST(BraveAdsSearchEngineResultsPageUtilTest, IsNotSearchEngineResultsPage) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(IsSearchEngineResultsPage(GURL("https://brave.com/")));
}

TEST(BraveAdsSearchEngineResultsPageUtilTest,
     IsNotSearchEngineResultsPageWithInvalidUrl) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(IsSearchEngineResultsPage(GURL("INVALID")));
}

TEST(BraveAdsSearchEngineResultsPageUtilTest, ExtractSearchTermQueryValue) {
  // Arrange

  // Act

  // Assert
  for (const auto& url : GetSearchEngineResultsPageUrls()) {
    const absl::optional<std::string> search_term_query_value =
        ExtractSearchTermQueryValue(url);
    if (search_term_query_value) {
      EXPECT_EQ("foobar", search_term_query_value);
    }
  }
}

TEST(BraveAdsSearchEngineResultsPageUtilTest,
     FailToExtractSearchTermQueryValueFromUrlWithMissingQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ExtractSearchTermQueryValue(GURL("https://google.com/")));
}

TEST(BraveAdsSearchEngineResultsPageUtilTest,
     FailToExtractSearchTermQueryValueFromInvalidUrl) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ExtractSearchTermQueryValue(GURL("INVALID")));
}

}  // namespace brave_ads
