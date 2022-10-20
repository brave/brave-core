/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/promoted_content_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";

constexpr char kJson[] =
    R"({"advertiser_id":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","campaign_id":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creative_instance_id":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creative_set_id":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","description":"Test Ad Description","segment":"untargeted","target_url":"https://brave.com/","title":"Test Ad Title","type":"promoted_content_ad","uuid":"f0948316-df6f-4e31-814d-d0b5f2a1f28c"})";

}  // namespace

class BatAdsPromotedContentAdValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsPromotedContentAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const PromotedContentAdInfo ad = PromotedContentAdFromValue(*dict);

  // Assert
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_guids*/ false);
  const PromotedContentAdInfo expected_ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad);
}

TEST_F(BatAdsPromotedContentAdValueUtilTest, ToValue) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_guids*/ false);
  const PromotedContentAdInfo ad =
      BuildPromotedContentAd(creative_ad, kPlacementId);

  // Act
  const base::Value::Dict value = PromotedContentAdToValue(ad);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
