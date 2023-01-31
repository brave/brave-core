/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::exclusion_rules::features {

TEST(BatAdsExclusionRuleFeaturesTest, IsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
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
  const bool is_enabled = IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
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
  const bool should_exclude_ad_if_converted = ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = false;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsExclusionRuleFeaturesTest, DefaultShouldExcludeAdIfConverted) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const bool should_exclude_ad_if_converted = ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = true;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsExclusionRuleFeaturesTest, DisabledShouldExcludeAdIfConverted) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool should_exclude_ad_if_converted = ShouldExcludeAdIfConverted();

  // Assert
  const bool expected_should_exclude_ad_if_converted = true;
  EXPECT_EQ(expected_should_exclude_ad_if_converted,
            should_exclude_ad_if_converted);
}

TEST(BatAdsExclusionRuleFeaturesTest, ExcludeAdIfDismissedWithinTimeWindow) {
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
  const base::TimeDelta time_window = ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Days(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsExclusionRuleFeaturesTest,
     DefaultExcludeAdIfDismissedWithinTimeWindow) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(0);
  EXPECT_EQ(expected_time_window, time_window);
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
  const base::TimeDelta time_window = ExcludeAdIfDismissedWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(0);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest, ExcludeAdIfTransferredWithinTimeWindow) {
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
  const base::TimeDelta time_window = ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Days(1);
  EXPECT_EQ(expected_time_window, time_window);
}

TEST(BatAdsUserActivityFeaturesTest,
     DefaultExcludeAdIfTransferredWithinTimeWindow) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const base::TimeDelta time_window = ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(0);
  EXPECT_EQ(expected_time_window, time_window);
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
  const base::TimeDelta time_window = ExcludeAdIfTransferredWithinTimeWindow();

  // Assert
  const base::TimeDelta expected_time_window = base::Hours(0);
  EXPECT_EQ(expected_time_window, time_window);
}

}  // namespace ads::exclusion_rules::features
