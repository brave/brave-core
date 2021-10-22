/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/sample_ads.h"

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_aliases.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSampleAdsTest, CalculateNormalisingConstantWithEmptyAds) {
  // Arrange
  const CreativeAdPredictorMap<CreativeAdNotificationInfo>
      creative_ad_predictors;

  // Act
  const double normalising_constant =
      CalculateNormalisingConstant(creative_ad_predictors);

  // Assert
  const double expected_normalising_constant = 0.0;
  EXPECT_TRUE(
      DoubleEquals(expected_normalising_constant, normalising_constant));
}

TEST(BatAdsSampleAdsTest, CalculateNormalisingConstant) {
  // Arrange
  CreativeAdPredictorMap<CreativeAdNotificationInfo> creative_ad_predictors;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 1.1;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_1;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 2.2;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_2;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 3.3;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_3;

  // Act
  const double normalising_constant =
      CalculateNormalisingConstant(creative_ad_predictors);

  // Assert
  const double expected_normalising_constant = 6.6;
  EXPECT_TRUE(
      DoubleEquals(expected_normalising_constant, normalising_constant));
}

TEST(BatAdsSampleAdsTest, SampleAdFromPredictorsWithZeroScores) {
  // Arrange
  CreativeAdPredictorMap<CreativeAdNotificationInfo> creative_ad_predictors;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_1;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 0;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_2;

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  creative_ad_predictors[base::GenerateGUID()] = ad_predictor_3;

  // Act
  const absl::optional<CreativeAdNotificationInfo> creative_ad_optional =
      SampleAdFromPredictors(creative_ad_predictors);

  // Assert
  const absl::optional<CreativeAdNotificationInfo>
      expected_creative_ad_optional = absl::nullopt;
  EXPECT_EQ(expected_creative_ad_optional, creative_ad_optional);
}

TEST(BatAdsSampleAdsTest,
     DeterministicallySampleAdFromPredictorsWithOneNonZeroScore) {
  // Arrange
  CreativeAdPredictorMap<CreativeAdNotificationInfo> creative_ad_predictors;

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification();
  creative_ad_1.segment = "foo-bar";

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.score = 0;
  ad_predictor_1.creative_ad = creative_ad_1;
  creative_ad_predictors[creative_ad_1.creative_instance_id] = ad_predictor_1;

  CreativeAdNotificationInfo creative_ad_2 = BuildCreativeAdNotification();
  creative_ad_2.segment = "foo-bar";

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.score = 0.1;
  ad_predictor_2.creative_ad = creative_ad_2;
  creative_ad_predictors[creative_ad_2.creative_instance_id] = ad_predictor_2;

  CreativeAdNotificationInfo creative_ad_3 = BuildCreativeAdNotification();
  creative_ad_3.segment = "foo-bar";

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_3;
  ad_predictor_3.score = 0;
  ad_predictor_3.creative_ad = creative_ad_3;
  creative_ad_predictors[creative_ad_3.creative_instance_id] = ad_predictor_3;

  // Act
  for (int i = 0; i < 10; i++) {
    const CreativeAdNotificationInfo expected_creative_ad = creative_ad_2;
    const absl::optional<CreativeAdNotificationInfo> creative_ad_optional =
        SampleAdFromPredictors(creative_ad_predictors);
    ASSERT_NE(absl::nullopt, creative_ad_optional);

    const CreativeAdNotificationInfo creative_ad = creative_ad_optional.value();

    EXPECT_EQ(expected_creative_ad, creative_ad);
  }

  // Assert
}

TEST(BatAdsSampleAdsTest, ProbabilisticallySampleAdFromPredictors) {
  // Arrange
  CreativeAdPredictorMap<CreativeAdNotificationInfo> creative_ad_predictors;

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification();
  creative_ad_1.segment = "foo-bar";

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_1;
  ad_predictor_1.creative_ad = creative_ad_1;
  ad_predictor_1.score = 3;
  creative_ad_predictors[creative_ad_1.creative_instance_id] = ad_predictor_1;

  CreativeAdNotificationInfo creative_ad_2 = BuildCreativeAdNotification();
  creative_ad_2.segment = "foo-bar";

  AdPredictorInfo<CreativeAdNotificationInfo> ad_predictor_2;
  ad_predictor_2.creative_ad = creative_ad_2;
  ad_predictor_2.score = 3;
  creative_ad_predictors[creative_ad_2.creative_instance_id] = ad_predictor_2;

  // Act
  int creative_ad_notification_1_count = 0;
  int creative_ad_notification_2_count = 0;

  // P(X>1) > 0.99999999 with X~Bin(n=25, p=0.5), i.e. less than 1 in 100M tests
  // are expected to fail
  for (int i = 0; i < 25; i++) {
    const absl::optional<CreativeAdNotificationInfo> creative_ad_optional =
        SampleAdFromPredictors(creative_ad_predictors);
    ASSERT_NE(absl::nullopt, creative_ad_optional);

    const CreativeAdNotificationInfo creative_ad = creative_ad_optional.value();

    if (creative_ad.creative_instance_id ==
        creative_ad_1.creative_instance_id) {
      creative_ad_notification_1_count++;
    } else if (creative_ad.creative_instance_id ==
               creative_ad_2.creative_instance_id) {
      creative_ad_notification_2_count++;
    }
  }

  // Assert
  EXPECT_FALSE(creative_ad_notification_1_count == 0 ||
               creative_ad_notification_2_count == 0);
}

}  // namespace ads
