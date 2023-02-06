/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/dismissed_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

constexpr const char* kCampaignIds[] = {"60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
                                        "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BatAdsDismissedExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsDismissedExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const AdEventList ad_events;

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin48HoursIfDismissed) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed,
      ConfirmationType::kDismissed,
  };

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()) -
                 base::Seconds(1));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin48HoursIfDismissedForMultipleTypes) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  AdEventList ad_events;

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kDismissed, Now());
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kDismissed, Now());
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kDismissed, Now());
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad, AdType::kSearchResultAd,
                   ConfirmationType::kDismissed, Now());
  ad_events.push_back(ad_event_4);

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin48HoursIfDismissedThenClicked) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kClicked};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()) -
                 base::Seconds(1));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter48HoursIfDismissedThenClicked) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kClicked};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin48HoursIfClickedThenDismissed) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kClicked,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()) -
                 base::Seconds(1));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter48HoursIfClickedThenDismissed) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kClicked,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter48HoursIfClickedThenDismissedTwice) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kClicked,
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       DoNotAllowAdWithSameCampaignIdWithin48HoursIfClickedThenDismissedTwice) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kClicked,
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()) -
                 base::Seconds(1));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithSameCampaignIdIfClickedThenDismissedTwiceWithin0Seconds) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "0s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kClicked,
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);
  }

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin48Hours) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "48h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad_2, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()) -
                 base::Seconds(1));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDismissedExclusionRuleTest,
       AllowAdWithDifferentCampaignIdAfter48Hours) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds[1];

  const std::vector<ConfirmationType> confirmation_types = {
      ConfirmationType::kViewed, ConfirmationType::kDismissed,
      ConfirmationType::kViewed, ConfirmationType::kDismissed};

  AdEventList ad_events;

  for (const auto& confirmation_type : confirmation_types) {
    const AdEventInfo ad_event = BuildAdEvent(
        creative_ad_2, AdType::kNotificationAd, confirmation_type, Now());
    ad_events.push_back(ad_event);

    AdvanceClockBy(base::Minutes(5));
  }

  AdvanceClockBy(base::Hours(48) -
                 (base::Minutes(5) * confirmation_types.size()));

  // Act
  DismissedExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
