/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/locale/subdivision_code_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSubdivisionCodeUtilTest,
    GetCountryCode) {
  // Arrange

  // Act
  const std::string country_code = locale::GetCountryCode("US-CA");

  // Assert
  EXPECT_EQ("US", country_code);
}

TEST(BatAdsSubdivisionCodeUtilTest,
    GetSubdivisionCode) {
  // Arrange

  // Act
  const std::string subdivision_code = locale::GetSubdivisionCode("US-CA");

  // Assert
  EXPECT_EQ("CA", subdivision_code);
}

}  // namespace ads
