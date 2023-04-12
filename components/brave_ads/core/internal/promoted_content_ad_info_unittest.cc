/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/promoted_content_ad_info.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsPromotedContentAdInfoTest : public UnitTestBase {};

TEST_F(BatAdsPromotedContentAdInfoTest, IsValid) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_guids*/ true);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);

  // Act

  // Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BatAdsPromotedContentAdInfoTest, IsInvalid) {
  // Arrange
  const PromotedContentAdInfo ad;

  // Act

  // Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace brave_ads
