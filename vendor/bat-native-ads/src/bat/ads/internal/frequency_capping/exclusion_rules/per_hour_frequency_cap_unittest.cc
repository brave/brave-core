/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";
}  // namespace

class BatAdsPerHourFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsPerHourFrequencyCapTest() = default;

  ~BatAdsPerHourFrequencyCapTest() override = default;
};

TEST_F(BatAdsPerHourFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  const AdEventList ad_events;

  // Act
  PerHourFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest, AdAllowedAfter1Hour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  PerHourFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest, AdAllowedAfter1HourForMultipleTypes) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event_1 =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 =
      GenerateAdEvent(AdType::kNewTabPageAd, ad, ConfirmationType::kServed);
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 = GenerateAdEvent(AdType::kPromotedContentAd, ad,
                                                 ConfirmationType::kServed);
  ad_events.push_back(ad_event_3);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  PerHourFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerHourFrequencyCapTest, DoNotAllowTheSameAdWithin1Hour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  PerHourFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
