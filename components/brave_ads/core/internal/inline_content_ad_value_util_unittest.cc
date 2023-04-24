/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/inline_content_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"({"advertiserId":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","campaignId":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","creativeSetId":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","ctaText":"Call to action text","description":"Test Ad Description","dimensions":"200x100","imageUrl":"https://brave.com/image","segment":"untargeted","targetUrl":"https://brave.com/","title":"Test Ad Title","type":"inline_content_ad","uuid":"8b742869-6e4a-490c-ac31-31b49130098a"})";

}  // namespace

class BraveAdsInlineContentAdValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsInlineContentAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value::Dict value = base::test::ParseJsonDict(kJson);

  // Act

  // Assert
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ false);
  const InlineContentAdInfo expected_ad =
      BuildInlineContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, InlineContentAdFromValue(value));
}

TEST_F(BraveAdsInlineContentAdValueUtilTest, ToValue) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ false);
  const InlineContentAdInfo ad =
      BuildInlineContentAd(creative_ad, kPlacementId);

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kJson), InlineContentAdToValue(ad));
}

}  // namespace brave_ads
