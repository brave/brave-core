/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/per_week_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPerWeekExclusionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsPerWeekExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 2;

  PerWeekExclusionRule exclusion_rule({});

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPerWeekExclusionRuleTest, AllowAdIfZero) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 0;

  PerWeekExclusionRule exclusion_rule({});

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPerWeekExclusionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  PerWeekExclusionRule exclusion_rule(ad_events);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPerWeekExclusionRuleTest, AllowAdIfDoesNotExceedCapAfter1Week) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  PerWeekExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(7));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPerWeekExclusionRuleTest, DoNotAllowAdIfExceedsCapWithin1Week) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  PerWeekExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(7) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPerWeekExclusionRuleTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_week = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  PerWeekExclusionRule exclusion_rule(ad_events);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
