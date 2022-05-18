/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/transferred_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/features/frequency_capping_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCreativeInstanceId[] = "9aea9a47-c6a0-4718-a0fa-706338bb2156";

const std::vector<std::string> kCampaignIds = {
    "60267cee-d5bb-4a0d-baaf-91cd7f18e07e",
    "90762cee-d5bb-4a0d-baaf-61cd7f18e07e"};

}  // namespace

class BatAdsTransferredExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsTransferredExclusionRuleTest() = default;

  ~BatAdsTransferredExclusionRuleTest() override = default;
};

TEST_F(BatAdsTransferredExclusionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds.at(0);

  const AdEventList ad_events;

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin48Hours) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "48h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds.at(0);

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kAdNotification,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(47));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdWithin48HoursForMultipleTypes) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "48h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds.at(0);

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_2, AdType::kAdNotification,
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

  task_environment_.FastForwardBy(base::Hours(47));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       DoNotAllowAdWithSameCampaignIdWithin48Hours) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "48h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(47));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdWithin0Seconds) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "0s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(47));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithSameCampaignIdAfter48Hours) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "48h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = kCreativeInstanceId;
  creative_ad.campaign_id = kCampaignIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(48));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsTransferredExclusionRuleTest,
       AllowAdWithDifferentCampaignIdAfter48Hours) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["exclude_ad_if_transferred_within_time_window"] = "48h";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_instance_id = kCreativeInstanceId;
  creative_ad_1.campaign_id = kCampaignIds.at(0);

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_instance_id = kCreativeInstanceId;
  creative_ad_2.campaign_id = kCampaignIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kAdNotification,
                   ConfirmationType::kTransferred, Now());

  ad_events.push_back(ad_event);

  task_environment_.FastForwardBy(base::Hours(48));

  // Act
  TransferredExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
