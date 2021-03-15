/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdServingFeaturesTest, AdServingEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsAdServingEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsAdServingFeaturesTest, AdServingDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::IsAdServingEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsAdServingFeaturesTest, DefaultAdNotificationsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 2;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DefaultDefaultAdNotificationsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 5;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DisabledDefaultAdNotificationsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 5;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

}  // namespace ads
