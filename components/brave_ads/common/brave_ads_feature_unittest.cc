/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/brave_ads_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsBraveAdsFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsBraveAdsFeatureEnabled());
}

TEST(BraveAdsBraveAdsFeatureTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kBraveAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsBraveAdsFeatureEnabled());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysRunService) {
  // Arrange
  base::FieldTrialParams params;
  params["should_always_run_service"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kBraveAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldAlwaysRunService.Get());
}

TEST(BraveAdsBraveAdsFeatureTest, DefaultShouldAlwaysRunService) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(kShouldAlwaysRunService.Get());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysRunServiceWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kBraveAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldAlwaysRunService.Get());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldLaunchAsInProcessService) {
  // Arrange
  base::FieldTrialParams params;
  params["should_launch_as_in_process_service"] = "true";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kBraveAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(kShouldLaunchAsInProcessRunService.Get());
}

TEST(BraveAdsBraveAdsFeatureTest, DefaultShouldLaunchAsInProcessService) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(kShouldLaunchAsInProcessRunService.Get());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldLaunchAsInProcessServiceWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kBraveAdsFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(kShouldLaunchAsInProcessRunService.Get());
}

}  // namespace brave_ads
