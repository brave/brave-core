/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"

#include <vector>

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCampaignIds = {
    "60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
    "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BatAdsDailyCapExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsDailyCapExclusionRuleTest() = default;

  ~BatAdsDailyCapExclusionRuleTest() override = default;
};

TEST_F(BatAdsDailyCapExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds.at(0);
  creative_ad.daily_cap = 2;

  const AdEventList ad_events;

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapExclusionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds.at(0);
  creative_ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapExclusionRuleTest,
       AllowAdIfDoesNotExceedCapForNoMatchingCampaigns) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.campaign_id = kCampaignIds.at(0);
  creative_ad_1.daily_cap = 1;

  CreativeAdInfo creative_ad_2;
  creative_ad_2.campaign_id = kCampaignIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad_2, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapExclusionRuleTest, AllowAdIfDoesNotExceedCapWithin1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds.at(0);
  creative_ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(23));

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapExclusionRuleTest, AllowAdIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds.at(0);
  creative_ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Days(1));

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDailyCapExclusionRuleTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds.at(0);
  creative_ad.daily_cap = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  DailyCapExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
