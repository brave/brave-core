/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/choose/sample_ads.h"

#include <vector>

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSampleAdsTest, CalculateNormalizingConstantBaseInteger) {
  // Arrange
  const std::vector<int> score = {1, 2, 3, 4, 5};

  // Act
  const int normalizing_constant = CalculateNormalizingConstant(score);

  // Assert
  EXPECT_EQ(15, normalizing_constant);
}

TEST(BraveAdsSampleAdsTest, CalculateNormalizingConstantBaseDouble) {
  // Arrange
  const std::vector<double> score = {1.3, 2.7, 3.1, 4.8, 5.2};

  // Act
  const double normalizing_constant = CalculateNormalizingConstant(score);

  // Assert
  EXPECT_DOUBLE_EQ(17.1, normalizing_constant);
}

TEST(BraveAdsSampleAdsTest, CalculateNormalizingConstantWithEmptyAds) {
  // Arrange
  const CreativeAdPredictorMap<CreativeNotificationAdInfo>
      creative_ad_predictors;

  // Act
  const double normalizing_constant =
      CalculateNormalizingConstantFromPredictors(creative_ad_predictors);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, normalizing_constant);
}

TEST(BraveAdsSampleAdsTest, CalculateNormalizingConstant) {
  // Arrange
  CreativeAdPredictorMap<CreativeNotificationAdInfo> creative_ad_predictors;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_1;
  ad_predictor_1.score = 1.1;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_1;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_2;
  ad_predictor_2.score = 2.2;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_2;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_3;
  ad_predictor_3.score = 3.3;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_3;

  // Act
  const double normalizing_constant =
      CalculateNormalizingConstantFromPredictors(creative_ad_predictors);

  // Assert
  EXPECT_DOUBLE_EQ(6.6, normalizing_constant);
}

TEST(BraveAdsSampleAdsTest, SampleAdFromPredictorsWithZeroScores) {
  // Arrange
  CreativeAdPredictorMap<CreativeNotificationAdInfo> creative_ad_predictors;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_1;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_2;
  ad_predictor_2.score = 0;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_2;

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  creative_ad_predictors[base::Uuid::GenerateRandomV4().AsLowercaseString()] =
      ad_predictor_3;

  // Act

  // Assert
  EXPECT_FALSE(SampleAdFromPredictors(creative_ad_predictors));
}

TEST(BraveAdsSampleAdsTest,
     DeterministicallySampleAdFromPredictorsWithOneNonZeroScore) {
  // Arrange
  CreativeAdPredictorMap<CreativeNotificationAdInfo> creative_ad_predictors;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  ad_predictor_1.creative_ad = creative_ad_1;
  creative_ad_predictors[creative_ad_1.creative_instance_id] = ad_predictor_1;

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_2;
  ad_predictor_2.score = 0.1;
  ad_predictor_2.creative_ad = creative_ad_2;
  creative_ad_predictors[creative_ad_2.creative_instance_id] = ad_predictor_2;

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  ad_predictor_3.creative_ad = creative_ad_3;
  creative_ad_predictors[creative_ad_3.creative_instance_id] = ad_predictor_3;

  // Act
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(creative_ad_2, SampleAdFromPredictors(creative_ad_predictors));
  }

  // Assert
}

TEST(BraveAdsSampleAdsTest, ProbabilisticallySampleAdFromPredictors) {
  // Arrange
  CreativeAdPredictorMap<CreativeNotificationAdInfo> creative_ad_predictors;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_1;
  ad_predictor_1.creative_ad = creative_ad_1;
  ad_predictor_1.score = 3;
  creative_ad_predictors[creative_ad_1.creative_instance_id] = ad_predictor_1;

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor_2;
  ad_predictor_2.creative_ad = creative_ad_2;
  ad_predictor_2.score = 3;
  creative_ad_predictors[creative_ad_2.creative_instance_id] = ad_predictor_2;

  // Act
  int creative_ad_1_count = 0;
  int creative_ad_2_count = 0;

  // P(X>1) > 0.99999999 with X~Bin(n=25, p=0.5), i.e. less than 1 in 100M tests
  // are expected to fail
  for (int i = 0; i < 25; i++) {
    const absl::optional<CreativeNotificationAdInfo> creative_ad =
        SampleAdFromPredictors(creative_ad_predictors);
    ASSERT_TRUE(creative_ad);

    if (creative_ad->creative_instance_id ==
        creative_ad_1.creative_instance_id) {
      creative_ad_1_count++;
    } else if (creative_ad->creative_instance_id ==
               creative_ad_2.creative_instance_id) {
      creative_ad_2_count++;
    }
  }

  // Assert
  EXPECT_FALSE((creative_ad_1_count == 0 || creative_ad_2_count == 0));
}

TEST(BraveAdsSampleAdsTest, ComputeProbabilities) {
  // Arrange
  const std::vector<int> scores = {1, 0, 5, 4};

  // Act
  const std::vector<double> probabilities = ComputeProbabilities(scores);

  // Assert
  const std::vector<double> expected_probabilities = {0.1, 0.0, 0.5, 0.4};
  EXPECT_EQ(expected_probabilities, probabilities);
}

}  // namespace brave_ads
