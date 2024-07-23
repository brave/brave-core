/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kInlineContentAdAsJson[] =
    R"(
        {
          "advertiserId": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
          "campaignId": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
          "creativeInstanceId": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
          "creativeSetId": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
          "ctaText": "Call to action text",
          "description": "Test Ad Description",
          "dimensions": "200x100",
          "imageUrl": "https://brave.com/image",
          "segment": "untargeted",
          "targetUrl": "https://brave.com/",
          "title": "Test Ad Title",
          "type": "inline_content_ad",
          "uuid": "9bac9ae4-693c-4569-9b3e-300e357780cf"
        })";

}  // namespace

class BraveAdsInlineContentAdValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsInlineContentAdValueUtilTest, InlineContentAdFromValue) {
  // Arrange
  const base::Value::Dict dict =
      base::test::ParseJsonDict(kInlineContentAdAsJson);

  // Act
  const InlineContentAdInfo ad = InlineContentAdFromValue(dict);

  // Assert
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(
          /*should_generate_random_uuids=*/false);
  EXPECT_EQ(BuildInlineContentAd(creative_ad, test::kPlacementId), ad);
}

TEST_F(BraveAdsInlineContentAdValueUtilTest, InlineContentAdToValue) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      test::BuildCreativeInlineContentAd(
          /*should_generate_random_uuids=*/false);
  const InlineContentAdInfo ad =
      BuildInlineContentAd(creative_ad, test::kPlacementId);

  // Act
  const base::Value::Dict dict = InlineContentAdToValue(ad);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kInlineContentAdAsJson), dict);
}

}  // namespace brave_ads
