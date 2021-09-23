/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/sample_ads.h"

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSampleAdsTest, CalculateNormalisingConstantWithEmptyAds) {
  // Arrange
  CreativeAdNotificationPredictorMap predictors;

  // Act
  const double normalising_constant = CalculateNormalisingConstant(predictors);

  // Assert
  const double expected_normalising_constant = 0.0;
  EXPECT_TRUE(
      DoubleEquals(expected_normalising_constant, normalising_constant));
}

TEST(BatAdsSampleAdsTest, CalculateNormalisingConstant) {
  // Arrange
  CreativeAdNotificationPredictorMap predictors;
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 1.1;
  predictors[base::GenerateGUID()] = ad_predictor_1;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 2.2;
  predictors[base::GenerateGUID()] = ad_predictor_2;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 3.3;
  predictors[base::GenerateGUID()] = ad_predictor_3;

  // Act
  const double normalising_constant = CalculateNormalisingConstant(predictors);

  // Assert
  const double expected_normalising_constant = 6.6;
  EXPECT_TRUE(
      DoubleEquals(expected_normalising_constant, normalising_constant));
}

TEST(BatAdsSampleAdsTest, SampleAdFromPredictorsWithZeroScores) {
  // Arrange
  CreativeAdNotificationPredictorMap predictors;
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  predictors[base::GenerateGUID()] = ad_predictor_1;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 0;
  predictors[base::GenerateGUID()] = ad_predictor_2;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  predictors[base::GenerateGUID()] = ad_predictor_3;

  // Act
  const absl::optional<CreativeAdNotificationInfo> ad =
      SampleAdFromPredictors(predictors);

  // Assert
  const absl::optional<CreativeAdNotificationInfo> expected_ad = absl::nullopt;
  EXPECT_EQ(expected_ad, ad);
}

TEST(BatAdsSampleAdsTest,
     DeterministicallySampleAdFromPredictorsWithOneNonZeroScore) {
  // Arrange
  CreativeAdNotificationPredictorMap predictors;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification("foo-bar", 1.0, 1);
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  ad_predictor_1.creative_ad = creative_ad_notification_1;
  predictors[creative_ad_notification_1.creative_instance_id] = ad_predictor_1;

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification("foo-bar", 1.0, 1);
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 0.1;
  ad_predictor_2.creative_ad = creative_ad_notification_2;
  predictors[creative_ad_notification_2.creative_instance_id] = ad_predictor_2;

  const CreativeAdNotificationInfo creative_ad_notification_3 =
      GetCreativeAdNotification("foo-bar", 1.0, 1);
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  ad_predictor_3.creative_ad = creative_ad_notification_3;
  predictors[creative_ad_notification_3.creative_instance_id] = ad_predictor_3;

  // Act
  for (int i = 0; i < 10; i++) {
    CreativeAdNotificationInfo expected_ad = creative_ad_notification_2;
    const absl::optional<CreativeAdNotificationInfo> ad =
        SampleAdFromPredictors(predictors);
    EXPECT_EQ(expected_ad, ad.value());
  }

  // Assert
}

TEST(BatAdsSampleAdsTest, ProbabilisticallySampleAdFromPredictors) {
  // Arrange
  CreativeAdNotificationPredictorMap predictors;

  const CreativeAdNotificationInfo creative_ad_notification_1 =
      GetCreativeAdNotification("foo-bar", 1.0, 1);
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.creative_ad = creative_ad_notification_1;
  ad_predictor_1.score = 3;
  predictors[creative_ad_notification_1.creative_instance_id] = ad_predictor_1;

  const CreativeAdNotificationInfo creative_ad_notification_2 =
      GetCreativeAdNotification("foo-bar", 1.0, 1);
  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.creative_ad = creative_ad_notification_2;
  ad_predictor_2.score = 3;
  predictors[creative_ad_notification_2.creative_instance_id] = ad_predictor_2;

  // Act
  int ads_1_count = 0;
  int ads_2_count = 0;

  // P(X>1) > 0.99999999 with X~Bin(n=25, p=0.5), i.e. less than 1 in 100M tests
  // are expected to fail
  for (int i = 0; i < 25; i++) {
    const absl::optional<CreativeAdNotificationInfo> ad =
        SampleAdFromPredictors(predictors);

    if (ad.value().creative_instance_id ==
        creative_ad_notification_1.creative_instance_id) {
      ads_1_count++;
    } else if (ad.value().creative_instance_id ==
               creative_ad_notification_2.creative_instance_id) {
      ads_2_count++;
    }
  }

  // Assert
  EXPECT_FALSE(ads_1_count == 0 || ads_2_count == 0);
}

}  // namespace ads
