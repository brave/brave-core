/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/marked_as_inappropriate_exclusion_rule.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/deprecated/client/client.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCreativeInstanceId[] = "9cf19f6e-25b8-44f1-9050-2a7247185489";
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";

}  // namespace

class BatAdsMarkedAsInappropriateExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsMarkedAsInappropriateExclusionRuleTest() = default;

  ~BatAdsMarkedAsInappropriateExclusionRuleTest() override = default;
};

TEST_F(BatAdsMarkedAsInappropriateExclusionRuleTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act
  MarkedAsInappropriateExclusionRule frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsMarkedAsInappropriateExclusionRuleTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.creative_set_id = kCreativeSetId;

  AdContentInfo ad_content;
  ad_content.creative_set_id = kCreativeSetId;
  ad_content.is_flagged = false;

  Client::Get()->ToggleFlaggedAd(ad_content);

  // Act
  MarkedAsInappropriateExclusionRule frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
