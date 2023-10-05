/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kJson[] =
    R"(
        {
          "advertiser_id": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
          "campaign_id": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
          "creative_instance_id": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
          "creative_set_id": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
          "description": "Test Ad Description",
          "segment": "untargeted",
          "target_url": "https://brave.com/",
          "title": "Test Ad Title",
          "type": "promoted_content_ad",
          "uuid": "9bac9ae4-693c-4569-9b3e-300e357780cf"
        })";

}  // namespace

class BraveAdsPromotedContentAdValueUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPromotedContentAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(kJson);

  // Act & Assert
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAdForTesting(
          /*should_use_random_uuids=*/false);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, PromotedContentAdFromValue(dict));
}

TEST_F(BraveAdsPromotedContentAdValueUtilTest, ToValue) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAdForTesting(
          /*should_use_random_uuids=*/false);
  const PromotedContentAdInfo ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(kJson), PromotedContentAdToValue(ad));
}

}  // namespace brave_ads
