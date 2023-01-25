/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/choose/eligible_ads_predictor_util.h"

#include <map>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsEligibleAdsPredictorUtilTest, GroupCreativeAdsByCreativeInstanceId) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.segment = "foo-bar2";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 = BuildCreativeNotificationAd();
  creative_ad_3.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 = BuildCreativeNotificationAd();
  creative_ad_4.creative_instance_id = creative_ad_2.creative_instance_id;
  creative_ad_4.segment = "foo-bar4";
  creative_ads.push_back(creative_ad_4);

  // Act
  const CreativeAdPredictorMap<CreativeNotificationAdInfo>
      creative_ad_predictors =
          GroupCreativeAdsByCreativeInstanceId(creative_ads);

  // Assert
  ASSERT_EQ(3U, creative_ad_predictors.size());

  const AdPredictorInfo<CreativeNotificationAdInfo>& ad_predictor =
      creative_ad_predictors.at(creative_ad_2.creative_instance_id);
  const SegmentList expected_segments = {"foo-bar2", "foo-bar4"};
  EXPECT_EQ(expected_segments, ad_predictor.segments);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     GroupCreativeAdsByCreativeInstanceIdForEmptyAds) {
  // Arrange
  const CreativeNotificationAdList creative_ads;

  // Act
  const CreativeAdPredictorMap<CreativeNotificationAdInfo>
      creative_ad_predictors =
          GroupCreativeAdsByCreativeInstanceId(creative_ads);

  // Assert
  EXPECT_TRUE(creative_ad_predictors.empty());
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithZeroWeightsNotAllowedByGriffin) {
  // Arrange
  std::map<std::string, std::string> params;
  params["ad_predictor_weights"] = "0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{features::kEligibleAds, params}}, {});

  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor;
  ad_predictor.creative_ad = creative_ad;
  ad_predictor.segments = {creative_ad.segment};
  ad_predictor.does_match_intent_child_segments = true;
  ad_predictor.does_match_intent_parent_segments = false;
  ad_predictor.does_match_interest_child_segments = false;
  ad_predictor.does_match_interest_parent_segments = false;
  ad_predictor.ad_last_seen_hours_ago = 15;
  ad_predictor.advertiser_last_seen_hours_ago = 48;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  EXPECT_LT(0, ad_predictor.score);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithDefaultWeights) {
  // Arrange
  CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  creative_ad.segment = "foo-bar";

  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor;
  ad_predictor.creative_ad = creative_ad;
  ad_predictor.segments = {creative_ad.segment};
  ad_predictor.does_match_intent_child_segments = true;
  ad_predictor.does_match_intent_parent_segments = false;
  ad_predictor.does_match_interest_child_segments = false;
  ad_predictor.does_match_interest_parent_segments = false;
  ad_predictor.ad_last_seen_hours_ago = 15;
  ad_predictor.advertiser_last_seen_hours_ago = 48;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  const double expected_score =
      0.0 + 1.0 + 1.0 * (15 / 24.0) + (1.0 / 2.0) * 1.0;
  EXPECT_EQ(expected_score, ad_predictor.score);
}

TEST(BatAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithEmptyAdFeatures) {
  // Arrange
  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  const double expected_score = 0.0;
  EXPECT_EQ(expected_score, ad_predictor.score);
}

}  // namespace ads
