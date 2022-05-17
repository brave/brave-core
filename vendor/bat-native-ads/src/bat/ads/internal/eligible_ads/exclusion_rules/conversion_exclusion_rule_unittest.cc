/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/features/frequency_capping_features.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCreativeSetIds = {
    "654f10df-fbc4-4a92-8d43-2edf73734a60",
    "465f10df-fbc4-4a92-8d43-4edf73734a60"};

}  // namespace

class BatAdsConversionExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsConversionExclusionRuleTest() = default;

  ~BatAdsConversionExclusionRuleTest() override = default;
};

TEST_F(BatAdsConversionExclusionRuleTest, AllowAdIfThereIsNoConversionHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = "654f10df-fbc4-4a92-8d43-2edf73734a60";

  const AdEventList ad_events;

  // Act
  ConversionExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest,
       DoNotAllowAdIfShouldNotAllowConversionTracking) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowConversionTracking,
                                   false);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest, DoNotAllowAdIfAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest,
       AllowAdIfAlreadyConvertedAndExclusionRuleDisabled) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_exclude_ad_if_converted"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kAdNotification,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsConversionExclusionRuleTest, AllowAdIfNotAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_set_id = kCreativeSetIds.at(0);

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_set_id = kCreativeSetIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_2, AdType::kAdNotification,
                   ConfirmationType::kConversion, Now());

  ad_events.push_back(ad_event);

  // Act
  ConversionExclusionRule frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
