/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_new_tab_page_ad_model_based_predictor_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeNewTabPageAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      0.0, kNewTabPageAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0,
                   kNewTabPageAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001, kNewTabPageAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNewTabPageAdModelBasedPredictorFeatureTest,
     LastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNewTabPageAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
