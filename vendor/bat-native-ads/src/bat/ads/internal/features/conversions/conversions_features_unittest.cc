/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/conversions/conversions_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsConversionsFeaturesTest, ConversionsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(features::IsConversionsEnabled());
}

TEST(BatAdsConversionsFeaturesTest, ConversionsResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, features::GetConversionsResourceVersion());
}

TEST(BatAdsConversionsFeaturesTest, DefaultConversionIdPattern) {
  // Arrange

  // Act

  // Assert
  std::string expected_pattern =
      "<meta.*name=\"ad-conversion-id\".*content=\"([^\"]*)\".*>";
  EXPECT_EQ(expected_pattern, features::GetGetDefaultConversionIdPattern());
}

}  // namespace ads
