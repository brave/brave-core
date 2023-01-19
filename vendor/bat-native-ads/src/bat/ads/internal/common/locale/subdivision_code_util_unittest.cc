/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/locale/subdivision_code_util.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSubdivisionCodeUtilTest, GetCountryCode) {
  // Arrange

  // Act
  const std::string country_code = locale::GetCountryCode("US-CA");

  // Assert
  EXPECT_EQ("US", country_code);
}

TEST(BatAdsSubdivisionCodeUtilTest, GetSubdivisionCode) {
  // Arrange

  // Act
  const std::string subdivision_code = locale::GetSubdivisionCode("US-CA");

  // Assert
  EXPECT_EQ("CA", subdivision_code);
}

TEST(BatAdsSubdivisionCodeUtilTest,
     IsSupportedCountryCodeForSubdivisionTargeting) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(locale::IsSupportedCountryCodeForSubdivisionTargeting("US"));
  EXPECT_TRUE(locale::IsSupportedCountryCodeForSubdivisionTargeting("CA"));
  EXPECT_FALSE(locale::IsSupportedCountryCodeForSubdivisionTargeting("ES"));
}

}  // namespace ads
