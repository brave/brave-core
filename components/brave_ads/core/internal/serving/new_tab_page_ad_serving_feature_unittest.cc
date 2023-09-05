/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNewTabPageAdServingFeatureTest, IsEnabled) {
  // Arrange

  // Act

  EXPECT_TRUE(IsNewTabPageAdServingFeatureEnabled());
}

TEST(BraveAdsNewTabPageAdServingFeatureTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kNewTabPageAdServingFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsNewTabPageAdServingFeatureEnabled());
}

TEST(BraveAdsNewTabPageAdServingFeatureTest, ServingVersion) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "0";
  enabled_features.emplace_back(kNewTabPageAdServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0, kNewTabPageAdServingVersion.Get());
}

TEST(BraveAdsNewTabPageAdServingFeatureTest, DefaultServingVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(2, kNewTabPageAdServingVersion.Get());
}

TEST(BraveAdsNewTabPageAdServingFeatureTest,
     DefaultServingVersionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kNewTabPageAdServingFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(2, kNewTabPageAdServingVersion.Get());
}

}  // namespace brave_ads
