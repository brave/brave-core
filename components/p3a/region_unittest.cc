/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/region.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

TEST(P3ARegionTest, GetRegionIdentifiers) {
  // Test valid country codes
  {
    auto result = GetRegionIdentifiers("US");
    EXPECT_EQ(result.region, "americas");
    EXPECT_EQ(result.sub_region, "northern-america");
  }

  {
    auto result = GetRegionIdentifiers("IN");
    EXPECT_EQ(result.region, "asia");
    EXPECT_EQ(result.sub_region, "southern-asia");
  }

  {
    auto result = GetRegionIdentifiers("FR");
    EXPECT_EQ(result.region, "europe");
    EXPECT_EQ(result.sub_region, "western-europe");
  }

  {
    auto result = GetRegionIdentifiers("AU");
    EXPECT_EQ(result.region, "oceania");
    EXPECT_EQ(result.sub_region, "oceania");
  }

  {
    auto result = GetRegionIdentifiers("ZA");
    EXPECT_EQ(result.region, "africa");
    EXPECT_EQ(result.sub_region, "subsaharan-africa");
  }

  // Test invalid country code
  {
    auto result = GetRegionIdentifiers("XX");
    EXPECT_EQ(result.region, "other");
    EXPECT_EQ(result.sub_region, "other");
  }

  // Test empty country code
  {
    auto result = GetRegionIdentifiers("");
    EXPECT_EQ(result.region, "other");
    EXPECT_EQ(result.sub_region, "other");
  }
}

}  // namespace p3a
