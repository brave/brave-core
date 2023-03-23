/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::exclusion_rules::features {

TEST(BatAdsExclusionRuleFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEnabled());
}

TEST(BatAdsExclusionRuleFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEnabled());
}

TEST(BatAdsExclusionRuleFeaturesTest, ShouldExcludeAdIfConverted) {
  // Arrange
  base::FieldTrialParams params;
  params["should_exclude_ad_if_converted"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(ShouldExcludeAdIfConverted());
}

TEST(BatAdsExclusionRuleFeaturesTest, DefaultShouldExcludeAdIfConverted) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(ShouldExcludeAdIfConverted());
}

TEST(BatAdsExclusionRuleFeaturesTest,
     DefaultShouldExcludeAdIfConvertedWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(ShouldExcludeAdIfConverted());
}

TEST(BatAdsExclusionRuleFeaturesTest, GetExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_dismissed_within_time_window"] = "24h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Days(1), GetExcludeAdIfDismissedWithinTimeWindow());
}

TEST(BatAdsExclusionRuleFeaturesTest,
     DefaultExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Hours(0), GetExcludeAdIfDismissedWithinTimeWindow());
}

TEST(BatAdsExclusionRuleFeaturesTest,
     DefaultExcludeAdIfDismissedWithinTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Hours(0), GetExcludeAdIfDismissedWithinTimeWindow());
}

TEST(BatAdsUserActivityFeaturesTest,
     GetExcludeAdIfTransferredWithinTimeWindow) {
  // Arrange
  base::FieldTrialParams params;
  params["exclude_ad_if_transferred_within_time_window"] = "24h";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Days(1), GetExcludeAdIfTransferredWithinTimeWindow());
}

TEST(BatAdsUserActivityFeaturesTest,
     DefaultExcludeAdIfTransferredWithinTimeWindow) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Hours(0), GetExcludeAdIfTransferredWithinTimeWindow());
}

TEST(BatAdsUserActivityFeaturesTest,
     DefaultExcludeAdIfTransferredWithinTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Hours(0), GetExcludeAdIfTransferredWithinTimeWindow());
}

}  // namespace brave_ads::exclusion_rules::features
