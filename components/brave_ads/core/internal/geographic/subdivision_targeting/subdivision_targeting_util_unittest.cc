/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionTargetingUtilTest, DoesSupportCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportCountryCode(
      /*United States of America*/ "US"));
  EXPECT_TRUE(DoesSupportCountryCode(/*Canada*/ "CA"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest, DoesNotSupportCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportCountryCode("XX"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest, DoesCountryCodeSupportSubdivision) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesCountryCodeSupportSubdivision(
      /*United States of America*/ "US", /**/ "US-CA"));
}

TEST(BraveAdsSubdivisionTargetingUtilTest,
     DoesNotCountryCodeSupportSubdivision) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesCountryCodeSupportSubdivision(
      /*United States of America*/ "US", /**/ "US-XX"));
}

}  // namespace brave_ads
