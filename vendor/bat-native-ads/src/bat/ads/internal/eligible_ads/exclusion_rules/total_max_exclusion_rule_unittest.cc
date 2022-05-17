/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/total_max_exclusion_rule.h"

#include <vector>

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCreativeSetIds = {
    "654f10df-fbc4-4a92-8d43-2edf73734a60",
    "465f10df-fbc4-4a92-8d43-4edf73734a60"};

}  // namespace

class BatAdsTotalMaxExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsTotalMaxExclusionRuleTest() = default;

  ~BatAdsTotalMaxExclusionRuleTest() override = default;
};

TEST_F(BatAdsTotalMaxExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  const AdEventList ad_events;

  // Act
  TotalMaxExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxExclusionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  // Act
  TotalMaxExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxExclusionRuleTest,
       AllowAdIfDoesNotExceedCapForNoMatchingCreatives) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_set_id = kCreativeSetIds.at(0);
  creative_ad_1.total_max = 2;

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_set_id = kCreativeSetIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad_2, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  TotalMaxExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTotalMaxExclusionRuleTest, DoNotAllowAdIfExceedsZeroCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 0;

  const AdEventList ad_events;

  // Act
  TotalMaxExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsTotalMaxExclusionRuleTest, DoNotAllowAdIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);
  creative_ad.total_max = 2;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kAdNotification, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  // Act
  TotalMaxExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
