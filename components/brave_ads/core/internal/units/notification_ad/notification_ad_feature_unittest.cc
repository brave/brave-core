/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNotificationAdFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsNotificationAdFeatureEnabled());
}

TEST(BraveAdsNotificationAdFeatureTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kNotificationAdFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsNotificationAdFeatureEnabled());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["default_ads_per_hour"] = "42";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(42, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultDefaultAdsPerHour) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(10, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultDefaultAdsPerHourWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kNotificationAdFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(10, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, MaximumAdsPerDay) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_day"] = "24";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(24, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultMaximumAdsPerDay) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(100, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultMaximumAdsPerDayWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kNotificationAdFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(100, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     CanFallbackToCustomNotificationAdsDefault) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(false, kCanFallbackToCustomNotificationAds.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, CanFallbackToCustomNotificationAds) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["can_fallback_to_custom_notifications"] = "true";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(true, kCanFallbackToCustomNotificationAds.Get());
}

}  // namespace brave_ads
