/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTextClassificationFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTextClassificationFeature));
}

TEST(BraveAdsTextClassificationFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextClassificationFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kTextClassificationFeature));
}

TEST(BraveAdsTextClassificationFeatureTest, TextClassificationResourceVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kTextClassificationFeature, {{"flatbuffers_resource_version", "0"}});

  // Act & Assert
  EXPECT_EQ(0, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     DefaultTextClassificationResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     DefaultTextClassificationResourceVersionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextClassificationFeature);

  // Act & Assert
  EXPECT_EQ(1, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     TextClassificationPageProbabilitiesHistorySize) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kTextClassificationFeature, {{"page_probabilities_history_size", "3"}});

  // Act & Assert
  EXPECT_EQ(3, kTextClassificationPageProbabilitiesHistorySize.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     DefaultTextClassificationPageProbabilitiesHistorySize) {
  // Act & Assert
  EXPECT_EQ(5, kTextClassificationPageProbabilitiesHistorySize.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     DefaultTextClassificationPageProbabilitiesHistorySizeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTextClassificationFeature);

  // Act & Assert
  EXPECT_EQ(5, kTextClassificationPageProbabilitiesHistorySize.Get());
}

}  // namespace brave_ads
