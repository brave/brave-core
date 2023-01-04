/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

#include "bat/ads/internal/ads/ad_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdInfoTest : public UnitTestBase {};

TEST_F(BatAdsAdInfoTest, IsValid) {
  // Arrange
  const AdInfo ad = BuildAd();

  // Act

  // Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BatAdsAdInfoTest, IsInvalid) {
  // Arrange
  const AdInfo ad;

  // Act

  // Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace ads
