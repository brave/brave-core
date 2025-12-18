/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/locale/locale_util.h"

#include "brave/components/search_query_metrics/locale/scoped_locale_for_testing.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsLocaleUtilTest, CurrentLanguageCode) {
  // Arrange
  const test::ScopedCurrentLanguageCode scoped_current_language_code{"en"};

  // Act & Assert
  EXPECT_EQ("en", CurrentLanguageCode());
}

}  // namespace metrics
