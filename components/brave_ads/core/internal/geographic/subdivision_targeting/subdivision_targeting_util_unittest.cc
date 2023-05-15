/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionTargetingUtilTest, DoesSupportSubdivisionTargeting) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DoesSupportSubdivisionTargeting(/*United States of America*/ "US"));
  EXPECT_TRUE(DoesSupportSubdivisionTargeting(/*Canada*/ "CA"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest, DoesNotSupportSubdivisionTargeting) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportSubdivisionTargeting("XX"));
}

}  // namespace brave_ads
