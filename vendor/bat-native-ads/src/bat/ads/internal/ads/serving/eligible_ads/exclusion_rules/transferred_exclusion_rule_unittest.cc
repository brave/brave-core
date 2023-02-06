/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"

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

class BatAdsTransferredExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsTransferredExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  const AdEventList ad_events;

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin48Hours) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "48h";
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

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(48) - base::Seconds(1));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin48HoursForMultipleTypes) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "48h";
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

  AdEventList ad_events;

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());
  ad_events.push_back(ad_event_1);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad_2, AdType::kNewTabPageAd,
                   ConfirmationType::kTransferred, Now());
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad_2, AdType::kPromotedContentAd,
                   ConfirmationType::kTransferred, Now());
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad_2, AdType::kSearchResultAd,
                   ConfirmationType::kTransferred, Now());
  ad_events.push_back(ad_event_3);

  AdvanceClockBy(base::Hours(48) - base::Seconds(1));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       DoNotAllowAdWithSameCampaignIdWithin48Hours) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "48h";
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

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(48) - base::Seconds(1));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin0Seconds) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "0s";
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

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(48) - base::Seconds(1));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter48Hours) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "48h";
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

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(48));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdAfter48Hours) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "48h";
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

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Hours(48));

  // Act
  TransferredExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
