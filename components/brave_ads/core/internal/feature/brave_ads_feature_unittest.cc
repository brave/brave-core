/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/feature/brave_ads_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsBraveAdsFeatureTest, ShouldLaunchAsInProcessService) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(
      kShouldLaunchBraveAdsAsAnInProcessServiceFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldLaunchAsInProcessService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotLaunchAsInProcessService) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldLaunchAsInProcessService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysRunService) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(kShouldAlwaysRunBraveAdsServiceFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldAlwaysRunService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysRunService) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldAlwaysRunService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysTriggerNewTabPageAdEvents) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldAlwaysTriggerNewTabPageAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysTriggerNewTabPageAdEvents) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldAlwaysTriggerNewTabPageAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldSupportSearchResultAds) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(kShouldSupportSearchResultAdsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldSupportSearchResultAds());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotSupportSearchResultAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldSupportSearchResultAds());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  enabled_features.emplace_back(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldAlwaysTriggerSearchResultAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysTriggerSearchResultAdEvents) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ShouldAlwaysTriggerSearchResultAdEvents());
}

}  // namespace brave_ads
