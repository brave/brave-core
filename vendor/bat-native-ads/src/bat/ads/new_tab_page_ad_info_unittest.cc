/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsNewTabPageAdInfoTest : public UnitTestBase {};

TEST_F(BatAdsNewTabPageAdInfoTest, IsValid) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd();
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act

  // Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BatAdsNewTabPageAdInfoTest, IsInvalid) {
  // Arrange
  const NewTabPageAdInfo ad;

  // Act

  // Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace ads
