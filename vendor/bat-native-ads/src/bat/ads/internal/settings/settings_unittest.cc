/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/settings/settings.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSettingsTest : public UnitTestBase {
 protected:
  BatAdsSettingsTest() = default;

  ~BatAdsSettingsTest() override = default;
};

TEST_F(BatAdsSettingsTest, AdsPerHourWhenUserHasChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.push_back({features::kAdServing, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, 3);

  // Act
  const uint64_t ads_per_hour = settings::GetAdsPerHour();

  // Assert
  const uint64_t expected_ads_per_hour = 3;

  EXPECT_EQ(expected_ads_per_hour, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, AdsPerHourWhenUserHasNotChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.push_back({features::kAdServing, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const uint64_t ads_per_hour = settings::GetAdsPerHour();

  // Assert
  const uint64_t expected_ads_per_hour = 2;

  EXPECT_EQ(expected_ads_per_hour, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, ClampMinAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "-1";
  enabled_features.push_back({features::kAdServing, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const uint64_t ads_per_hour = settings::GetAdsPerHour();

  // Assert
  const uint64_t expected_ads_per_hour = 0;

  EXPECT_EQ(expected_ads_per_hour, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, ClampMaxAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "11";
  enabled_features.push_back({features::kAdServing, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const uint64_t ads_per_hour = settings::GetAdsPerHour();

  // Assert
  const uint64_t expected_ads_per_hour = 10;

  EXPECT_EQ(expected_ads_per_hour, ads_per_hour);
}

}  // namespace ads
