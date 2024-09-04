/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daily_cap_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr const char* kCampaignIds[] = {"60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
                                        "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BraveAdsDailyCapExclusionRuleTest : public test::TestBase {};

TEST_F(BraveAdsDailyCapExclusionRuleTest, ShouldIncludeIfThereAreNoAdEvents) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds[0];
  creative_ad.daily_cap = 2;

  const DailyCapExclusionRule exclusion_rule(/*ad_events=*/{});

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDailyCapExclusionRuleTest, ShouldIncludeIfDoesNotExceedCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds[0];
  creative_ad.daily_cap = 2;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kServedImpression,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const DailyCapExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDailyCapExclusionRuleTest,
       ShouldIncludeIfDoesNotExceedCapForNoMatchingCampaigns) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.campaign_id = kCampaignIds[0];
  creative_ad_1.daily_cap = 1;

  CreativeAdInfo creative_ad_2;
  creative_ad_2.campaign_id = kCampaignIds[1];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kServedImpression, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const DailyCapExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsDailyCapExclusionRuleTest,
       ShouldIncludeIfDoesNotExceedCapWithin1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds[0];
  creative_ad.daily_cap = 2;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kServedImpression,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const DailyCapExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDailyCapExclusionRuleTest,
       ShouldIncludeIfDoesNotExceedCapAfter1Day) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds[0];
  creative_ad.daily_cap = 2;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kServedImpression,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const DailyCapExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDailyCapExclusionRuleTest, ShouldExcludeIfExceedsCap) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = kCampaignIds[0];
  creative_ad.daily_cap = 2;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kServedImpression,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  const DailyCapExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
