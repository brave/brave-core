/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/subdivision_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionUtilTest, GetSubdivisionCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("US", GetSubdivisionCountryCode(/*subdivision*/ "US-CA"));
}

TEST(BraveAdsSubdivisionUtilTest, DoNotGetSubdivisionCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(GetSubdivisionCountryCode({}));
}

TEST(BraveAdsSubdivisionUtilTest, GetSubdivisionCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("CA", GetSubdivisionCode(/*subdivision*/ "US-CA"));
}

TEST(BraveAdsSubdivisionUtilTest, DoNotGetSubdivisionCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(GetSubdivisionCode({}));
}

}  // namespace brave_ads
