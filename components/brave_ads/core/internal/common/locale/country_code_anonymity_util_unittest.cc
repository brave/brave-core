/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/country_code_anonymity_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::locale {

TEST(BraveAdsCountryCodeUtilTest, IsCountryCodeMemberOfAnonymitySet) {
  // Act & Assert
  EXPECT_TRUE(IsCountryCodeMemberOfAnonymitySet("US"));
}

TEST(BraveAdsCountryCodeUtilTest, IsCountryCodeNotMemberOfAnonymitySet) {
  // Act & Assert
  EXPECT_FALSE(IsCountryCodeMemberOfAnonymitySet("XX"));
}

TEST(BraveAdsCountryCodeUtilTest, ShouldClassifyCountryCodeAsOther) {
  // Act & Assert
  EXPECT_TRUE(ShouldClassifyCountryCodeAsOther("CX"));
}

TEST(BraveAdsCountryCodeUtilTest, ShouldNotClassifyCountryCodeAsOther) {
  // Act & Assert
  EXPECT_FALSE(ShouldClassifyCountryCodeAsOther("XX"));
}

}  // namespace brave_ads::locale
