/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionTargetingUtilTest, ShouldTargetSubdivisionCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(ShouldTargetSubdivisionCountryCode(
      /*United States of America*/ "US"));
  EXPECT_TRUE(ShouldTargetSubdivisionCountryCode(/*Canada*/ "CA"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest,
     ShouldNotTargetSubdivisionCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldTargetSubdivisionCountryCode("XX"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest, ShouldTargetSubdivision) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(ShouldTargetSubdivision(
      /*United States of America*/ "US", /*subdivision*/ "US-CA"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest, ShouldNotTargetSubdivision) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldTargetSubdivision(
      /*United States of America*/ "US", /*subdivision*/ "US-XX"));
}

}  // namespace brave_ads
