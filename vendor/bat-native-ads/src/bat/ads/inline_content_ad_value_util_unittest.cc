/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";

constexpr char kJson[] =
    R"({"advertiserId":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","campaignId":"84197fc8-830a-4a8e-8339-7a70c2bfa104","creativeInstanceId":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creativeSetId":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","ctaText":"Call to action text","description":"Test Ad Description","dimensions":"200x100","imageUrl":"https://brave.com/image","segment":"untargeted","targetUrl":"https://brave.com/","title":"Test Ad Title","type":"inline_content_ad","uuid":"f0948316-df6f-4e31-814d-d0b5f2a1f28c"})";

}  // namespace

class BatAdsInlineContentAdValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsInlineContentAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const InlineContentAdInfo ad = InlineContentAdFromValue(*dict);

  // Assert
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ false);
  const InlineContentAdInfo expected_ad =
      BuildInlineContentAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad);
}

TEST_F(BatAdsInlineContentAdValueUtilTest, ToValue) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_guids*/ false);
  const InlineContentAdInfo ad =
      BuildInlineContentAd(creative_ad, kPlacementId);

  // Act
  const base::Value::Dict value = InlineContentAdToValue(ad);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
