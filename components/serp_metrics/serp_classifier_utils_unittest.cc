/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier_utils.h"

#include "base/containers/fixed_flat_set.h"
#include "components/search_engines/search_engine_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

TEST(SerpClassifierUtilsTest, IsAllowedSearchEngine) {
  constexpr auto kAllowedSearchEngines =
      base::MakeFixedFlatSet<SearchEngineType>(
          base::sorted_unique,
          {SEARCH_ENGINE_BING, SEARCH_ENGINE_GOOGLE, SEARCH_ENGINE_YAHOO,
           SEARCH_ENGINE_DUCKDUCKGO, SEARCH_ENGINE_QWANT, SEARCH_ENGINE_ECOSIA,
           SEARCH_ENGINE_BRAVE, SEARCH_ENGINE_STARTPAGE});

  for (int i = 0; i <= SEARCH_ENGINE_MAX; ++i) {
    const SearchEngineType search_engine_type =
        static_cast<SearchEngineType>(i);
    EXPECT_EQ(IsAllowedSearchEngine(search_engine_type),
              kAllowedSearchEngines.contains(search_engine_type));
  }
}

}  // namespace serp_metrics
