/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";
}  // namespace

class BatAdsNewTabPageAdsPerHourFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsNewTabPageAdsPerHourFrequencyCapTest() = default;

  ~BatAdsNewTabPageAdsPerHourFrequencyCapTest() override = default;
};

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
    AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  const AdEventList ad_events;

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap(ad_events);
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
    AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kNewTabPageAd, ad,
      ConfirmationType::kViewed);

  const AdEventList ad_events(kNewTabPageAdsPerHourFrequencyCap - 1, ad_event);

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap(ad_events);
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
    AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kNewTabPageAd, ad,
      ConfirmationType::kViewed);

  const AdEventList ad_events(kNewTabPageAdsPerHourFrequencyCap, ad_event);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap(ad_events);
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
    DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_instance_id = kCreativeInstanceId;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kNewTabPageAd, ad,
      ConfirmationType::kViewed);

  const AdEventList ad_events(kNewTabPageAdsPerHourFrequencyCap, ad_event);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap(ad_events);
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
    AdsPerHour) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(4UL, kNewTabPageAdsPerHourFrequencyCap);
}

}  // namespace ads
