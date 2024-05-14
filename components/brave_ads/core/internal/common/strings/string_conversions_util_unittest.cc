/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsStringConversionsUtilTest, TrueBoolToString) {
  // Act & Assert
  EXPECT_EQ("true", BoolToString(true));
}

TEST(BraveAdsStringConversionsUtilTest, FalseBoolToString) {
  // Act & Assert
  EXPECT_EQ("false", BoolToString(false));
}

}  // namespace brave_ads
