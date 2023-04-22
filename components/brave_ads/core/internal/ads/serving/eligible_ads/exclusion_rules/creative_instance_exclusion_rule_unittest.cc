/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeInstanceExclusionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  CreativeInstanceExclusionRule exclusion_rule({});

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest, AdAllowedAfter1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       AdAllowedAfter1HourForMultipleTypes) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_4);

  CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       DoNotAllowTheSameAdWithin1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());

  ad_events.push_back(ad_event);

  CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
