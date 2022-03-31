/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_month_exclusion_rule.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsPerMonthExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsPerMonthExclusionRuleTest() = default;

  ~BatAdsPerMonthExclusionRuleTest() override = default;
};

TEST_F(BatAdsPerMonthExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 2;

  const AdEventList ad_events;

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthExclusionRuleTest, AllowAdIfZero) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 0;

  const AdEventList ad_events;

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthExclusionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthExclusionRuleTest, AllowAdIfDoesNotExceedCapAfter1Month) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::Days(28));

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthExclusionRuleTest, DoNotAllowAdIfExceedsCapWithin1Month) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::Days(27));

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerMonthExclusionRuleTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  PerMonthExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
