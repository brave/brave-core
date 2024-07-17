/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/sampling/creative_ad_model_based_predictor_sampling.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorSamplingTest
    : public test::TestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorSamplingTest, SampleCreativeAd) {
  // Arrange
  CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      creative_ad_predictors;

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor;
  creative_ad_predictor.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor.score = 1.0;
  creative_ad_predictors.push_back(creative_ad_predictor);

  // Act & Assert
  EXPECT_EQ(creative_ad_predictor.creative_ad,
            MaybeSampleCreativeAd(creative_ad_predictors));
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorSamplingTest,
       DeterministicallySampleCreativeAdWhenOneNonZeroScore) {
  // Arrange
  CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      creative_ad_predictors;

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor_1;
  creative_ad_predictor_1.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor_1.score = 0.0;
  creative_ad_predictors.push_back(creative_ad_predictor_1);

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor_2;
  creative_ad_predictor_2.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor_2.score = 1.0;
  creative_ad_predictors.push_back(creative_ad_predictor_2);

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor_3;
  creative_ad_predictor_3.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor_3.score = 0.0;
  creative_ad_predictors.push_back(creative_ad_predictor_3);

  // Act & Assert
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(creative_ad_predictor_2.creative_ad,
              MaybeSampleCreativeAd(creative_ad_predictors));
  }
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorSamplingTest,
       ProbabilisticallySampleCreativeAd) {
  // Arrange
  CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      creative_ad_predictors;

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor_1;
  creative_ad_predictor_1.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor_1.score = 3.0;
  creative_ad_predictors.push_back(creative_ad_predictor_1);

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor_2;
  creative_ad_predictor_2.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor_2.score = 3.0;
  creative_ad_predictors.push_back(creative_ad_predictor_2);

  // Act & Assert
  int creative_ad_1_count = 0;
  int creative_ad_2_count = 0;

  for (int i = 0; i < 25; ++i) {
    // P(X>1) > 0.99999999 with X~Bin(n=25, p=0.5), i.e. less than 1 in 100M
    // tests are expected to fail.
    const std::optional<CreativeNotificationAdInfo> creative_ad =
        MaybeSampleCreativeAd(creative_ad_predictors);
    ASSERT_TRUE(creative_ad);

    if (creative_ad->creative_instance_id ==
        creative_ad_predictor_1.creative_ad.creative_instance_id) {
      ++creative_ad_1_count;
    } else if (creative_ad->creative_instance_id ==
               creative_ad_predictor_2.creative_ad.creative_instance_id) {
      ++creative_ad_2_count;
    }
  }

  EXPECT_NE(0, creative_ad_1_count);
  EXPECT_NE(0, creative_ad_2_count);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorSamplingTest,
       DoNotSampleCreativeAd) {
  // Arrange
  CreativeAdModelBasedPredictorList<CreativeNotificationAdInfo>
      creative_ad_predictors;

  CreativeAdModelBasedPredictorInfo<CreativeNotificationAdInfo>
      creative_ad_predictor;
  creative_ad_predictor.creative_ad =
      test::BuildCreativeNotificationAd(/*should_generate_random_uuids=*/true);
  creative_ad_predictor.score = 0.0;
  creative_ad_predictors.push_back(creative_ad_predictor);

  // Act & Assert
  EXPECT_FALSE(MaybeSampleCreativeAd(creative_ad_predictors));
}

}  // namespace brave_ads
