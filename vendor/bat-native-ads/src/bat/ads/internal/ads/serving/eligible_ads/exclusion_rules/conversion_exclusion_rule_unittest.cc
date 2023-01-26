/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr const char* kCreativeSetIds[] = {
    "654f10df-fbc4-4a92-8d43-2edf73734a60",
    "465f10df-fbc4-4a92-8d43-4edf73734a60"};

}  // namespace

class BatAdsConversionExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsConversionExclusionRuleTest, AllowAdIfThereIsNoConversionHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = "654f10df-fbc4-4a92-8d43-2edf73734a60";

  const AdEventList ad_events;

  // Act
  ConversionExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest, DoNotAllowAdIfAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds[0];

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest,
       AllowAdIfAlreadyConvertedAndExclusionRuleDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_converted"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(exclusion_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds[0];

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kNotificationAd,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest, AllowAdIfNotAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_set_id = kCreativeSetIds[0];

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_set_id = kCreativeSetIds[1];

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule exclusion_rule(ad_events);
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
