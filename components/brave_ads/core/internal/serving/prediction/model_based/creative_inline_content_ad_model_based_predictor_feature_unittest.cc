/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_inline_content_ad_model_based_predictor_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeInlineContentAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kInlineContentAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kInlineContentAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kInlineContentAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeInlineContentAdModelBasedPredictorFeatureTest,
     LastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kInlineContentAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
