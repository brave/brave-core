/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"

#include <vector>

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCreativeSetIds = {
    "654f10df-fbc4-4a92-8d43-2edf73734a60",
    "465f10df-fbc4-4a92-8d43-4edf73734a60"};

}  // namespace

class BatAdsTotalMaxFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsTotalMaxFrequencyCapTest() = default;

  ~BatAdsTotalMaxFrequencyCapTest() override = default;
};

TEST_F(BatAdsTotalMaxFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  const AdEventList ad_events;

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxFrequencyCapTest,
       AllowAdIfDoesNotExceedCapForMultipleTypes) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

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

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxFrequencyCapTest,
       AllowAdIfDoesNotExceedCapForNoMatchingCreatives) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_set_id = kCreativeSetIds.at(0);
  creative_ad_1.total_max = 2;

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_set_id = kCreativeSetIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad_2, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxFrequencyCapTest, DoNotAllowAdIfExceedsZeroCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 0;

  const AdEventList ad_events;

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsTotalMaxFrequencyCapTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(
      AdType::kAdNotification, creative_ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  TotalMaxFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
