/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr const char* kCampaignIds[] = {"60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
                                        "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BraveAdsTransferredExclusionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsTransferredExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds[0];

  TransferredExclusionRule exclusion_rule({});

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin2Days) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "2d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin2DaysForMultipleTypes) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "2d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       DoNotAllowAdWithSameCampaignIdWithin2Days) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "2d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin0Seconds) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "0s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2) - base::Milliseconds(1));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter2Days) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "2d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdAfter2Days) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_transferred_within_time_window"] = "2d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kExclusionRulesFeature, params);

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

  TransferredExclusionRule exclusion_rule(ad_events);

  AdvanceClockBy(base::Days(2));

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

}  // namespace brave_ads
