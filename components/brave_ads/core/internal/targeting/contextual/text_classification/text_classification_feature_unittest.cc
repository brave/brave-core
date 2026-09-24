/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTextClassificationFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTextClassificationFeature));
}

TEST(BraveAdsTextClassificationFeatureTest, TextClassificationResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kTextClassificationResourceVersion.Get());
}

TEST(BraveAdsTextClassificationFeatureTest,
     TextClassificationPageProbabilitiesHistorySize) {
  // Act & Assert
  EXPECT_EQ(15U, kTextClassificationPageProbabilitiesHistorySize.Get());
}

}  // namespace brave_ads
