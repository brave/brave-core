/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_util.h"

#include "brave/components/brave_ads/core/public/common/locale/scoped_locale_for_testing.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdsUtilTest, IsSupportedRegion) {
  // Arrange
  const test::ScopedCurrentCountryCode scoped_current_country_code{"US"};

  // Act & Assert
  EXPECT_TRUE(IsSupportedRegion());
}

TEST(BraveAdsAdsUtilTest, IsUnsupportedRegion) {
  // Arrange
  const test::ScopedCurrentCountryCode scoped_current_country_code{
      /*cuba*/ "CU"};

  // Act & Assert
  EXPECT_FALSE(IsSupportedRegion());
}

}  // namespace brave_ads
