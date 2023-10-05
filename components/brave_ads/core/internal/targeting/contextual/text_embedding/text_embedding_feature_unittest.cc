/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTextEmbeddingFeatureTest, IsEnabled) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kTextEmbeddingFeature);

  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTextEmbeddingFeature));
}

TEST(BraveAdsTextEmbeddingFeatureTest, IsDisabled) {
  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kTextEmbeddingFeature));
}

TEST(BraveAdsTextEmbeddingFeatureTest, TextEmbeddingResourceVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kTextEmbeddingFeature, {{"resource_version", "0"}});

  // Act & Assert
  EXPECT_EQ(0, kTextEmbeddingResourceVersion.Get());
}

TEST(BraveAdsTextEmbeddingFeatureTest, DefaultTextEmbeddingResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kTextEmbeddingResourceVersion.Get());
}

TEST(BraveAdsTextEmbeddingFeatureTest,
     DefaultTextEmbeddingResourceVersionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextEmbeddingFeature);

  // Act & Assert
  EXPECT_EQ(1, kTextEmbeddingResourceVersion.Get());
}

TEST(BraveAdsTextEmbeddingFeatureTest, TextEmbeddingsHistorySize) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kTextEmbeddingFeature, {{"history_size", "42"}});

  // Act & Assert
  EXPECT_EQ(42, kTextEmbeddingHistorySize.Get());
}

TEST(BraveAdsTextEmbeddingFeatureTest, DefaultTextEmbeddingsHistorySize) {
  // Act & Assert
  EXPECT_EQ(10, kTextEmbeddingHistorySize.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     DefaultTextEmbeddingsHistorySizeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextEmbeddingFeature);

  // Act & Assert
  EXPECT_EQ(10, kTextEmbeddingHistorySize.Get());
}

}  // namespace brave_ads
