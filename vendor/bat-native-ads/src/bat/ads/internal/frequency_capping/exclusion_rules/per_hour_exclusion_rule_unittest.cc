/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_exclusion_rule.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";
}  // namespace

class BatAdsPerHourExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsPerHourExclusionRuleTest() = default;

  ~BatAdsPerHourExclusionRuleTest() override = default;
};

TEST_F(BatAdsPerHourExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  const AdEventList ad_events;

  // Act
  PerHourExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourExclusionRuleTest, AdAllowedAfter1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  FastForwardClockBy(base::Hours(1));

  // Act
  PerHourExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourExclusionRuleTest, AdAllowedAfter1HourForMultipleTypes) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event_1 = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 = GenerateAdEvent(
      AdType::kNewTabPageAd, creative_ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 = GenerateAdEvent(
      AdType::kPromotedContentAd, creative_ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 = GenerateAdEvent(
      AdType::kSearchResultAd, creative_ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_4);

  FastForwardClockBy(base::Hours(1));

  // Act
  PerHourExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourExclusionRuleTest, DoNotAllowTheSameAdWithin1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  FastForwardClockBy(base::Minutes(59));

  // Act
  PerHourExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
