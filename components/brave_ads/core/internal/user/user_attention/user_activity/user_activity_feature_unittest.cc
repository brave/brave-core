/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_attention/user_activity/user_activity_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserActivityFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsUserActivityFeatureEnabled());
}

TEST(BraveAdsUserActivityFeatureTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kUserActivityFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsUserActivityFeatureEnabled());
}

TEST(BraveAdsUserActivityFeatureTest, Triggers) {
  // Arrange
  base::FieldTrialParams params;
  params["triggers"] = "01=0.5;010203=1.0;0203=0.75";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserActivityFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ("01=0.5;010203=1.0;0203=0.75", kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultTriggers) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0",
      kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultTriggersWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kUserActivityFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0",
      kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, TimeWindow) {
  // Arrange
  base::FieldTrialParams params;
  params["time_window"] = "2h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserActivityFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Hours(2), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultTimeWindow) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(15), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kUserActivityFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(15), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, Threshold) {
  // Arrange
  base::FieldTrialParams params;
  params["threshold"] = "7.0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserActivityFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(7.0, kUserActivityThreshold.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultThreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(0.0, kUserActivityThreshold.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kUserActivityFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0.0, kUserActivityThreshold.Get());
}

}  // namespace brave_ads
