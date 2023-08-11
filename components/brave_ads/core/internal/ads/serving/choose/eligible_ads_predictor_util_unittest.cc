/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/choose/eligible_ads_predictor_util.h"

#include <map>
#include <numeric>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     GroupCreativeAdsByCreativeInstanceId) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.segment = "foo-bar1";
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.segment = "foo-bar2";
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.segment = "foo-bar3";
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
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

TEST(BraveAdsEligibleAdsPredictorUtilTest,
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

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithZeroWeightsNotAllowedByGriffin) {
  // Arrange
  std::map<std::string, std::string> params;
  params["ad_predictor_weights"] = "0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{kEligibleAdFeature, params}}, {});

  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
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

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithDefaultWeights) {
  // Arrange
  CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
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
  EXPECT_EQ(0.0 + 1.0 + 1.0 * (15 / 24.0) + (1.0 / 2.0) * 1.0,
            ad_predictor.score);
}

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     ComputePredictorScoreWithEmptyAdPredictor) {
  // Arrange
  AdPredictorInfo<CreativeNotificationAdInfo> ad_predictor;

  // Act
  ad_predictor.score = ComputePredictorScore(ad_predictor);

  // Assert
  EXPECT_EQ(0.0, ad_predictor.score);
}

TEST(BraveAdsEligibleAdsPredictorUtilTest, ComputeVoteRegistry) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {-0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.embedding = {-0.0853, 0.1789, -0.4221};
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_4.creative_instance_id = creative_ad_2.creative_instance_id;
  creative_ad_4.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_4);

  TextEmbeddingHtmlEventList text_embeddings;

  const TextEmbeddingHtmlEventInfo text_embedding_event_1 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());
  text_embeddings.push_back(text_embedding_event_1);

  const TextEmbeddingHtmlEventInfo text_embedding_event_2 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());
  text_embeddings.push_back(text_embedding_event_2);

  // Act
  const std::vector<int> vote_registry =
      ComputeVoteRegistry(creative_ads, text_embeddings);

  // Assert
  ASSERT_EQ(creative_ads.size(), vote_registry.size());

  const int total_votes =
      std::accumulate(vote_registry.begin(), vote_registry.end(), 0);
  EXPECT_EQ(static_cast<int>(text_embeddings.size()), total_votes);
  EXPECT_EQ(static_cast<int>(text_embeddings.size()),
            vote_registry.at(creative_ads.size() - 1));
}

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     ComputeVoteRegistryWithMultipleCreativesWithSameEmbeddings) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_3);

  TextEmbeddingHtmlEventList text_embeddings;

  const TextEmbeddingHtmlEventInfo text_embedding_event_1 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());
  text_embeddings.push_back(text_embedding_event_1);

  // Act
  const std::vector<int> vote_registry =
      ComputeVoteRegistry(creative_ads, text_embeddings);

  // Assert
  ASSERT_EQ(creative_ads.size(), vote_registry.size());

  const int total_votes =
      std::accumulate(vote_registry.begin(), vote_registry.end(), 0);
  EXPECT_EQ(3 * static_cast<int>(text_embeddings.size()), total_votes);
  EXPECT_EQ(vote_registry.at(0), vote_registry.at(creative_ads.size() - 1));
  EXPECT_EQ(static_cast<int>(text_embeddings.size()),
            vote_registry.at(creative_ads.size() - 1));
}

TEST(BraveAdsEligibleAdsPredictorUtilTest,
     ComputeVoteRegistryWithNoEmbeddingHistory) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {-0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_3.embedding = {-0.0853, 0.1789, -0.4221};
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_4.creative_instance_id = creative_ad_2.creative_instance_id;
  creative_ad_4.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_4);

  TextEmbeddingHtmlEventList text_embeddings;

  // Act
  const std::vector<int> vote_registry =
      ComputeVoteRegistry(creative_ads, text_embeddings);

  // Assert
  ASSERT_EQ(creative_ads.size(), vote_registry.size());

  const int total_votes =
      std::accumulate(vote_registry.begin(), vote_registry.end(), 0);
  EXPECT_EQ(static_cast<int>(text_embeddings.size()), total_votes);
  EXPECT_EQ(static_cast<int>(text_embeddings.size()),
            vote_registry.at(creative_ads.size() - 1));
}

}  // namespace brave_ads
