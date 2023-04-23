/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/subdivision_code_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSubdivisionCodeUtilTest, GetCountryCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("US", locale::GetCountryCode("US-CA"));
}

TEST(BraveAdsSubdivisionCodeUtilTest, GetSubdivisionCode) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("CA", locale::GetSubdivisionCode("US-CA"));
}

}  // namespace brave_ads
