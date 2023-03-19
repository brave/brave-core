/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::targeting::features {

TEST(BatAdsTextClassificationFeaturesTest, TextClassificationEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsTextClassificationEnabled());
}

TEST(BatAdsTextClassificationFeaturesTest,
     TextClassificationProbabilitiesHistorySize) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(5, GetTextClassificationProbabilitiesHistorySize());
}

TEST(BatAdsTextClassificationFeaturesTest, TextClassificationResource) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, GetTextClassificationResourceVersion());
}

}  // namespace brave_ads::targeting::features
