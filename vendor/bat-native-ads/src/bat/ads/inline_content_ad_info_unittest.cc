/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_info.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsInlineContentAdInfoTest : public UnitTestBase {};

TEST_F(BatAdsInlineContentAdInfoTest, IsValid) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd();
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act

  // Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BatAdsInlineContentAdInfoTest, IsInvalid) {
  // Arrange
  const InlineContentAdInfo ad;

  // Act

  // Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace ads
