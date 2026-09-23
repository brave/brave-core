/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_notification_ad_model_based_predictor_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(
      kCreativeNotificationAdModelBasedPredictorFeature));
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0, kNotificationAdChildIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentIntentSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentIntentSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdChildLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentLatentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(
      1.0, kNotificationAdParentLatentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ChildInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdChildInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     ParentInterestSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(1.0,
                   kNotificationAdParentInterestSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     UntargetedSegmentAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0001,
                   kNotificationAdUntargetedSegmentPredictorWeight.Get());
}

TEST(BraveAdsCreativeNotificationAdModelBasedPredictorFeatureTest,
     LastSeenAdPredictorWeight) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kNotificationAdLastSeenPredictorWeight.Get());
}

}  // namespace brave_ads
