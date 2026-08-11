// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/url_filter_validation.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace traffic_control {

TEST(UrlFilterValidationTest, AcceptsCommonFilters) {
  EXPECT_TRUE(IsValidUrlFilter("example.com"));
  EXPECT_TRUE(IsValidUrlFilter(".example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com"));
  EXPECT_TRUE(IsValidUrlFilter("https://example.com/path"));
  EXPECT_TRUE(IsValidUrlFilter("*"));
}

TEST(UrlFilterValidationTest, RejectsInvalidFilters) {
  EXPECT_FALSE(IsValidUrlFilter(""));
  EXPECT_FALSE(IsValidUrlFilter("http://"));
}

}  // namespace traffic_control
