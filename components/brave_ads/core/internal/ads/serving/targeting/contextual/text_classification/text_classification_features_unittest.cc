/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::targeting {

TEST(BraveAdsTextClassificationFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsTextClassificationEnabled());
}

TEST(BraveAdsTextClassificationFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kTextClassificationFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsTextClassificationEnabled());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     GetTextClassificationResourceVersion) {
  // Arrange
  base::FieldTrialParams params;
  params["resource_version"] = "0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kTextClassificationFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     DefaultTextClassificationResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     DefaultTextClassificationResourceVersionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kTextClassificationFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(1, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     GetTextClassificationProbabilitiesHistorySize) {
  // Arrange
  base::FieldTrialParams params;
  params["page_probabilities_history_size"] = "3";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kTextClassificationFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(3, kTextClassificationPageProbabilitiesHistorySize.Get());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     DefaultTextClassificationProbabilitiesHistorySize) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(5, kTextClassificationPageProbabilitiesHistorySize.Get());
}

TEST(BraveAdsTextClassificationFeaturesTest,
     DefaultTextClassificationProbabilitiesHistorySizeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kTextClassificationFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(5, kTextClassificationPageProbabilitiesHistorySize.Get());
}

}  // namespace brave_ads::targeting
