/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/page_land_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr const char* kCampaignIds[] = {"60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
                                        "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BraveAdsPageLandExclusionRuleTest : public test::TestBase {};

TEST_F(BraveAdsPageLandExclusionRuleTest, ShouldAlwaysInclude) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "0h"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const PageLandExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest, ShouldIncludeIfThereAreNoAdEvents) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const PageLandExclusionRule exclusion_rule(/*ad_events=*/{});

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldIncludeWithDifferentCampaignIdWithin2Days) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "2d"}});

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);
  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldIncludeWithDifferentCampaignIdWithin2DaysForMultipleAdTypes) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "2d"}});

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  AdEventList ad_events;

  const AdEventInfo ad_event_1 = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNewTabPageAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kPromotedContentAd,
      mojom::ConfirmationType::kLanded,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kSearchResultAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event_3);

  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldExcludeWithSameCampaignIdWithin2Days) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(
      kExclusionRulesFeature,
      base::FieldTrialParams(
          {{"should_exclude_ad_if_landed_on_page_within_time_window", "2d"}}));
  enabled_features.emplace_back(
      kSiteVisitFeature, base::FieldTrialParams({{"page_land_cap", "1"}}));
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    /*disabled_features=*/{});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdWithin0Seconds) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "0s"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdAfter2Days) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsPageLandExclusionRuleTest,
       ShouldIncludeWithDifferentCampaignIdAfter2Days) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_landed_on_page_within_time_window", "2d"}});

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kLanded, /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const PageLandExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

}  // namespace brave_ads
