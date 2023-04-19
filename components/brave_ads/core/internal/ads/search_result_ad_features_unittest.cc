/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/search_result_ad_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::search_result_ads {

TEST(BatAdsFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEnabled());
}

TEST(BatAdsFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEnabled());
}

TEST(BatAdsFeaturesTest, GetMaximumAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_hour"] = "42";
  enabled_features.emplace_back(kAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(42, kMaximumAdsPerHour.Get());
}

TEST(BatAdsFeaturesTest, DefaultMaximumAdsPerHour) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(10, kMaximumAdsPerHour.Get());
}

TEST(BatAdsFeaturesTest, DefaultMaximumAdsPerHourWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(10, kMaximumAdsPerHour.Get());
}

TEST(BatAdsFeaturesTest, GetMaximumAdsPerDay) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["maximum_ads_per_day"] = "42";
  enabled_features.emplace_back(kAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(42, kMaximumAdsPerDay.Get());
}

TEST(BatAdsFeaturesTest, DefaultMaximumAdsPerDay) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(40, kMaximumAdsPerDay.Get());
}

TEST(BatAdsFeaturesTest, DefaultMaximumAdsPerDayWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(40, kMaximumAdsPerDay.Get());
}

}  // namespace brave_ads::search_result_ads
