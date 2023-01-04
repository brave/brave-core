/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_results_page_util.h"

#include <vector>

#include "bat/ads/internal/common/search_engine/search_engine_results_page_unittest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSearchEngineResultsPageUtilTest, IsSearchEngineResultsPage) {
  // Arrange
  const std::vector<GURL>& urls = GetSearchEngineResultsPageUrls();

  // Act
  for (const auto& url : urls) {
    EXPECT_TRUE(IsSearchEngineResultsPage(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineResultsPageUtilTest, IsNotSearchEngineResultsPage) {
  // Arrange
  const GURL url = GURL("https://brave.com/");

  // Act
  const bool is_search_engine_result_page = IsSearchEngineResultsPage(url);

  // Assert
  EXPECT_FALSE(is_search_engine_result_page);
}

TEST(BatAdsSearchEngineResultsPageUtilTest,
     IsNotSearchEngineResultsPageWithInvalidUrl) {
  // Arrange
  const GURL url = GURL("INVALID_URL");

  // Act
  const bool is_search_engine_result_page = IsSearchEngineResultsPage(url);

  // Assert
  EXPECT_FALSE(is_search_engine_result_page);
}

TEST(BatAdsSearchEngineResultsPageUtilTest, ExtractSearchTermQueryValue) {
  // Arrange
  const std::vector<GURL>& urls = GetSearchEngineResultsPageUrls();

  // Act
  for (const auto& url : urls) {
    const absl::optional<std::string> search_term_query_value =
        ExtractSearchTermQueryValue(url);
    if (search_term_query_value) {
      EXPECT_EQ("foobar", *search_term_query_value);
    }
  }

  // Assert
}

TEST(BatAdsSearchEngineResultsPageUtilTest,
     FailToExtractSearchTermQueryValueFromUrlWithMissingQuery) {
  // Arrange
  const GURL url = GURL("https://google.com/");

  // Act
  const absl::optional<std::string> search_term_query_value =
      ExtractSearchTermQueryValue(url);

  // Assert
  EXPECT_FALSE(search_term_query_value);
}

TEST(BatAdsSearchEngineResultsPageUtilTest,
     FailToExtractSearchTermQueryValueFromInvalidUrl) {
  // Arrange
  const GURL url = GURL("INVALID_URL");

  // Act
  const absl::optional<std::string> search_term_query_value =
      ExtractSearchTermQueryValue(url);

  // Assert
  EXPECT_FALSE(search_term_query_value);
}

}  // namespace ads
