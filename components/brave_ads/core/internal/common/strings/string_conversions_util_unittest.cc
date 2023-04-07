/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsStringConversionsUtilTest, TrueBoolToString) {
  // Arrange

  // Act
  const std::string value = BoolToString(true);

  // Assert
  EXPECT_EQ("true", value);
}

TEST(BatAdsStringConversionsUtilTest, FalseBoolToString) {
  // Arrange

  // Act
  const std::string value = BoolToString(false);

  // Assert
  EXPECT_EQ("false", value);
}

}  // namespace brave_ads
