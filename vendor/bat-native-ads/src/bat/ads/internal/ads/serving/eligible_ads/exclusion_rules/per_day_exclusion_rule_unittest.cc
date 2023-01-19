/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/per_day_exclusion_rule.h"

#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsPerDayExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsPerDayExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 2;

  const AdEventList ad_events;

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayExclusionRuleTest, AllowAdIfZero) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 0;

  const AdEventList ad_events;

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayExclusionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayExclusionRuleTest, AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Days(1));

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayExclusionRuleTest, DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(24) - base::Seconds(1));

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerDayExclusionRuleTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  PerDayExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
