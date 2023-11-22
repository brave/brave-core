/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting.h"

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorVotingTest,
     ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents) {
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
  creative_ad_3.embedding = {-0.0853, 0.1789, -0.4221};
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_4.creative_instance_id = creative_ad_2.creative_instance_id;
  creative_ad_4.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_4);

  TextEmbeddingHtmlEventList text_embedding_html_events;

  const TextEmbeddingHtmlEventInfo text_embedding_html_event_1 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());
  text_embedding_html_events.push_back(text_embedding_html_event_1);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event_2 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());
  text_embedding_html_events.push_back(text_embedding_html_event_2);

  // Act & Assert
  const std::vector<int> expected_creative_ad_vote_registry = {0, 2, 0, 2};
  EXPECT_EQ(expected_creative_ad_vote_registry,
            ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
                creative_ads, text_embedding_html_events));
}

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorVotingTest,
     ComputeCreativeAdVoteRegistryForSameTextEmbeddingHtmlEvents) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_2);

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_3);

  TextEmbeddingHtmlEventList text_embedding_html_events;

  const TextEmbeddingHtmlEventInfo text_embedding_html_event_1 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());
  text_embedding_html_events.push_back(text_embedding_html_event_1);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event_2 =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());
  text_embedding_html_events.push_back(text_embedding_html_event_2);

  // Act & Assert
  const std::vector<int> expected_creative_ad_vote_registry = {2, 2, 2};
  EXPECT_EQ(expected_creative_ad_vote_registry,
            ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
                creative_ads, text_embedding_html_events));
}

TEST(BraveAdsCreativeAdEmbeddingBasedPredictorVotingTest,
     ComputeCreativeAdVoteRegistryForNoTextEmbeddingHtmlEvents) {
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
  creative_ad_3.embedding = {-0.0853, 0.1789, -0.4221};
  creative_ads.push_back(creative_ad_3);

  CreativeNotificationAdInfo creative_ad_4 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_4.creative_instance_id = creative_ad_2.creative_instance_id;
  creative_ad_4.embedding = {0.0853, -0.1789, 0.4221};
  creative_ads.push_back(creative_ad_4);

  // Act & Assert
  const std::vector<int> expected_creative_ad_vote_registry = {0, 0, 0, 0};
  EXPECT_EQ(expected_creative_ad_vote_registry,
            ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
                creative_ads, /*text_embedding_html_events=*/{}));
}

}  // namespace brave_ads
