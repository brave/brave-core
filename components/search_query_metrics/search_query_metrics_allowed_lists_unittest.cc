/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_allowed_lists.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsSearchEngineUtilTest, AllowedCountries) {
  EXPECT_THAT(kAllowedCountries,
              ::testing::ElementsAreArray({"BR", "CA", "CO", "DE", "ES", "FR",
                                           "GB", "IN", "IT", "JP", "MX", "NL",
                                           "PH", "PL", "US"}));
}

TEST(SearchQueryMetricsSearchEngineUtilTest, AllowedLanguages) {
  EXPECT_THAT(kAllowedLanguages,
              ::testing::ElementsAreArray({"de", "en", "es", "fr", "hi", "it",
                                           "ja", "nl", "pl", "pt", "tl"}));
}

TEST(SearchQueryMetricsSearchEngineUtilTest, AllowedDefaultSearchEngines) {
  EXPECT_THAT(kAllowedDefaultSearchEngines,
              ::testing::ElementsAreArray({"Bing", "Brave", "DuckDuckGo",
                                           "Ecosia", "Google", "Qwant",
                                           "Startpage", "Yahoo! JAPAN"}));
}

TEST(SearchQueryMetricsSearchEngineUtilTest, AllowedSearchEngines) {
  EXPECT_THAT(kAllowedSearchEngines,
              ::testing::ElementsAreArray(
                  {"Bing", "Brave", "ChatGPT", "DuckDuckGo", "Ecosia", "Google",
                   "Perplexity", "Qwant", "Startpage", "Yahoo! JAPAN"}));
}

}  // namespace metrics
