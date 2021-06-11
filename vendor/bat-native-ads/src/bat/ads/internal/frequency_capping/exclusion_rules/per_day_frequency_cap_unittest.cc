/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsPerDayFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsPerDayFrequencyCapTest() = default;

  ~BatAdsPerDayFrequencyCapTest() override = default;
};

TEST_F(BatAdsPerDayFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  const AdEventList ad_events;

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest, AllowAdIfZero) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 0;

  const AdEventList ad_events;

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest,
       AllowAdIfDoesNotExceedCapForMultipleTypes) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  AdEventList ad_events;

  AdEventInfo ad_event_1 =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_1);

  AdEventInfo ad_event_2 =
      GenerateAdEvent(AdType::kNewTabPageAd, ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_2);

  AdEventInfo ad_event_3 = GenerateAdEvent(AdType::kPromotedContentAd, ad,
                                           ConfirmationType::kServed);
  ad_events.push_back(ad_event_3);

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest, AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromDays(1));

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest, DoNotAllowAdIfExceedsCapWithin1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromHours(23));

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerDayFrequencyCapTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_day = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  PerDayFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
