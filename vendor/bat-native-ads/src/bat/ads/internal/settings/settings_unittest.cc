/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/settings/settings.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsSettingsTest : public UnitTestBase {};

TEST_F(BatAdsSettingsTest, AdsPerHourWhenUserHasChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.emplace_back(features::kServing, kParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, 3);

  // Act
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();

  // Assert
  EXPECT_EQ(3, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, AdsPerHourWhenUserHasNotChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.emplace_back(features::kServing, kParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();

  // Assert
  EXPECT_EQ(2, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, ClampMinAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "-1";
  enabled_features.emplace_back(features::kServing, kParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();

  // Assert
  EXPECT_EQ(0, ads_per_hour);
}

TEST_F(BatAdsSettingsTest, ClampMaxAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kParameters;
  kParameters["default_ad_notifications_per_hour"] = "11";
  enabled_features.emplace_back(features::kServing, kParameters);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();

  // Assert
  EXPECT_EQ(10, ads_per_hour);
}

}  // namespace ads
