/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_dismissed_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr const char* kCampaignIds[] = {"60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
                                        "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BraveAdsDismissedExclusionRuleTest : public test::TestBase {};

TEST_F(BraveAdsDismissedExclusionRuleTest, ShouldAlwaysInclude) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "0h"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kDismissed,
      /*created_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest, ShouldIncludeIfThereAreNoAdEvents) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const NotificationAdDismissedExclusionRule exclusion_rule(/*ad_events=*/{});

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdWithin2DaysIfDismissedOnce) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression,
      ConfirmationType::kDismissed,
  };

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(
    BraveAdsDismissedExclusionRuleTest,
    ShouldIncludeWithSameCampaignIdWithin2DaysIfDismissedOnceForMultipleAdTypes) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;

  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad_1, ConfirmationType::kDismissed, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);

  const AdInfo ad_2 = test::BuildAd(AdType::kNewTabPageAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad_2, ConfirmationType::kDismissed, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  const AdInfo ad_3 = test::BuildAd(AdType::kPromotedContentAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_3 = BuildAdEvent(
      ad_3, ConfirmationType::kDismissed, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_3);

  const AdInfo ad_4 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  const AdEventInfo ad_event_4 = BuildAdEvent(
      ad_4, ConfirmationType::kDismissed, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_4);

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdWithin2DaysIfDismissedThenClicked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdAfter2DaysIfDismissedThenClicked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdWithin2DaysIfClickedThenDismissed) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdAfter2DaysIfClickedThenDismissed) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdAfter2DaysIfClickedThenDismissedTwice) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldExcludeWithSameCampaignIdWithin2DaysIfClickedThenDismissedTwice) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithSameCampaignIdIfClickedThenDismissedTwiceWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "0s"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kClicked,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithDifferentCampaignIdWithin2Days) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_dismissed_within_time_window", "2d"}});

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsDismissedExclusionRuleTest,
       ShouldIncludeWithDifferentCampaignIdAfter2Days) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = test::kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed,
      ConfirmationType::kViewedImpression, ConfirmationType::kDismissed};

  AdEventList ad_events;
  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event =
        test::BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                           confirmation_type, /*created_at=*/test::Now(),
                           /*should_generate_random_uuids=*/true);
    ad_events.push_back(ad_event);
    AdvanceClockBy(base::Minutes(5));
  }

  const NotificationAdDismissedExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

}  // namespace brave_ads
