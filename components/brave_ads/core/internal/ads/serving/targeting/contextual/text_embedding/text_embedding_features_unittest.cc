/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::targeting {

TEST(BatAdsTextEmbeddingFeaturesTest, IsEnabled) {
  // Arrange
  base::FieldTrialParams params;
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kTextEmbeddingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_TRUE(IsTextEmbeddingEnabled());
}

TEST(BatAdsTextEmbeddingFeaturesTest, IsDisabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(IsTextEmbeddingEnabled());
}

TEST(BatAdsTextEmbeddingFeaturesTest, GetTextEmbeddingResourceVersion) {
  // Arrange
  base::FieldTrialParams params;
  params["resource_version"] = "0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kTextEmbeddingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0, kTextEmbeddingResourceVersion.Get());
}

TEST(BatAdsTextEmbeddingFeaturesTest, DefaultTextEmbeddingResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, kTextEmbeddingResourceVersion.Get());
}

TEST(BatAdsTextEmbeddingFeaturesTest,
     DefaultTextEmbeddingResourceVersionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kTextEmbeddingFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(1, kTextEmbeddingResourceVersion.Get());
}

TEST(BatAdsTextEmbeddingFeaturesTest, GetTextEmbeddingsHistorySize) {
  // Arrange
  base::FieldTrialParams params;
  params["history_size"] = "42";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kTextEmbeddingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(42, kTextEmbeddingHistorySize.Get());
}

TEST(BatAdsTextEmbeddingFeaturesTest, DefaultTextEmbeddingsHistorySize) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(10, kTextEmbeddingHistorySize.Get());
}

TEST(BatAdsTextClassificationFeaturesTest,
     DefaultTextEmbeddingsHistorySizeWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kTextEmbeddingFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(10, kTextEmbeddingHistorySize.Get());
}

}  // namespace brave_ads::targeting
