/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"

#include <vector>

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCampaignIds = {
    "60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
    "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BatAdsDailyCapFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsDailyCapFrequencyCapTest() = default;

  ~BatAdsDailyCapFrequencyCapTest() override = default;
};

TEST_F(BatAdsDailyCapFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  const AdEventList ad_events;

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
       AllowAdIfDoesNotExceedCapForMultipleTypes) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

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

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest,
       AllowAdIfDoesNotExceedCapForNoMatchingCampaigns) {
  // Arrange
  CreativeAdInfo ad_1;
  ad_1.campaign_id = kCampaignIds.at(0);
  ad_1.daily_cap = 1;

  CreativeAdInfo ad_2;
  ad_2.campaign_id = kCampaignIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad_2, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest, AllowAdIfDoesNotExceedCapWithin1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::TimeDelta::FromHours(23));

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest, AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(1));

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapFrequencyCapTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo ad;
  ad.campaign_id = kCampaignIds.at(0);
  ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event =
      GenerateAdEvent(AdType::kAdNotification, ad, ConfirmationType::kServed);

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  DailyCapFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
