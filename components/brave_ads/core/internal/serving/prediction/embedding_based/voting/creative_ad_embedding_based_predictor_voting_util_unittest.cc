/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting_util.h"

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/scoring/creative_ad_embedding_based_predictor_scoring.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorVotingUtilTest,
     ComputeCreativeAdVoteRegistryForSimilarityScores) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.embedding = {-0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.embedding = {0.0253, -0.1189, 0.3621};
  creative_ads.push_back(creative_ad_3);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());

  const std::vector<double> creative_ad_similarity_scores =
      ComputeCreativeAdSimilarityScores(creative_ads,
                                        text_embedding_html_event);

  std::vector<int> creative_ad_vote_registry(creative_ads.size());

  // Act
  ComputeCreativeAdVoteRegistryForSimilarityScores(
      creative_ad_similarity_scores, creative_ad_vote_registry);

  // Assert
  const std::vector<int> expected_creative_ad_vote_registry = {0, 1, 0};
  EXPECT_EQ(expected_creative_ad_vote_registry, creative_ad_vote_registry);
}

}  // namespace brave_ads
