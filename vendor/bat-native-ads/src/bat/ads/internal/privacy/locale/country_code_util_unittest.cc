/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/locale/country_code_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace locale {

TEST(BatAdsCountryCodeUtilTest, IsMemberOfAnonymitySet) {
  // Arrange

  // Act
  const bool is_member_of_anonymity_set = IsMemberOfAnonymitySet("en-US");

  // Assert
  EXPECT_TRUE(is_member_of_anonymity_set);
}

TEST(BatAdsCountryCodeUtilTest, IsNotMemberOfAnonymitySet) {
  // Arrange

  // Act
  const bool is_member_of_anonymity_set = IsMemberOfAnonymitySet("en-XX");

  // Assert
  EXPECT_FALSE(is_member_of_anonymity_set);
}

TEST(BatAdsCountryCodeUtilTest, ShouldClassifyAsOther) {
  // Arrange

  // Act
  const bool is_anonymous = ShouldClassifyAsOther("en-CX");

  // Assert
  EXPECT_TRUE(is_anonymous);
}

TEST(BatAdsCountryCodeUtilTest, ShouldNotClassifyAsOther) {
  // Arrange

  // Act
  const bool is_anonymous = ShouldClassifyAsOther("en-XX");

  // Assert
  EXPECT_FALSE(is_anonymous);
}

}  // namespace locale
}  // namespace privacy
}  // namespace ads
