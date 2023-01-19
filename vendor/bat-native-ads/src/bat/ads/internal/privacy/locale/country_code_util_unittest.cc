/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/locale/country_code_util.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::locale {

TEST(BatAdsCountryCodeUtilTest, IsCountryCodeMemberOfAnonymitySet) {
  // Arrange

  // Act
  const bool is_member_of_anonymity_set =
      IsCountryCodeMemberOfAnonymitySet("US");

  // Assert
  EXPECT_TRUE(is_member_of_anonymity_set);
}

TEST(BatAdsCountryCodeUtilTest, IsCountryCodeNotMemberOfAnonymitySet) {
  // Arrange

  // Act
  const bool is_member_of_anonymity_set =
      IsCountryCodeMemberOfAnonymitySet("XX");

  // Assert
  EXPECT_FALSE(is_member_of_anonymity_set);
}

TEST(BatAdsCountryCodeUtilTest, ShouldClassifyCountryCodeAsOther) {
  // Arrange

  // Act
  const bool is_anonymous = ShouldClassifyCountryCodeAsOther("CX");

  // Assert
  EXPECT_TRUE(is_anonymous);
}

TEST(BatAdsCountryCodeUtilTest, ShouldNotClassifyCountryCodeAsOther) {
  // Arrange

  // Act
  const bool is_anonymous = ShouldClassifyCountryCodeAsOther("XX");

  // Assert
  EXPECT_FALSE(is_anonymous);
}

}  // namespace ads::privacy::locale
