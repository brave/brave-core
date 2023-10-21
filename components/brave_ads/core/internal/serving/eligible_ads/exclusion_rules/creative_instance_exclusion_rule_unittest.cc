/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/creative_instance_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeInstanceExclusionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       ShouldIncludeIfThereAreNoAdEvents) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  const CreativeInstanceExclusionRule exclusion_rule(/*ad_events=*/{});

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest, ShouldIncludeAfter1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       ShouldIncludeAfter1HourForMultipleAdTypes) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;

  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 = test::BuildAdEvent(
      creative_ad, AdType::kPromotedContentAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 = test::BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_4);

  const CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsCreativeInstanceExclusionRuleTest,
       ShouldExcludeTheSameAdWithin1Hour) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeInstanceExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
