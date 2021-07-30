/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/per_month_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsPerMonthFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsPerMonthFrequencyCapTest() = default;

  ~BatAdsPerMonthFrequencyCapTest() override = default;
};

TEST_F(BatAdsPerMonthFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 2;

  const AdEventList ad_events;

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthFrequencyCapTest, AllowAdIfZero) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 0;

  const AdEventList ad_events;

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthFrequencyCapTest, AllowAdIfDoesNotExceedCapAfter1Month) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromDays(28));

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsPerMonthFrequencyCapTest, DoNotAllowAdIfExceedsCapWithin1Month) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  FastForwardClockBy(base::TimeDelta::FromDays(27));

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsPerMonthFrequencyCapTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.per_month = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  PerMonthFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
