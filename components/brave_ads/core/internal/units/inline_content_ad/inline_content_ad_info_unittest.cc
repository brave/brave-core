/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInlineContentAdInfoTest : public UnitTestBase {};

TEST_F(BraveAdsInlineContentAdInfoTest, IsValid) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(/*should_use_random_uuids=*/true);
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act & Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BraveAdsInlineContentAdInfoTest, IsInvalid) {
  // Arrange
  const InlineContentAdInfo ad;

  // Act & Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace brave_ads
