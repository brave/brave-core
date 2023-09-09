/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/sampling/creative_ad_embedding_based_predictor_sampling.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/creative_ad_embedding_based_predictor_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/voting/creative_ad_embedding_based_predictor_voting.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdEmbeddingBasedPredictorSamplingTest
    : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdEmbeddingBasedPredictorSamplingTest,
       SampleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {-0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {0.0853, -0.1789, 0.4221};
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

  TextEmbeddingHtmlEventList text_embedding_html_events;
  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::BuildTextEmbeddingForTesting());
  text_embedding_html_events.push_back(text_embedding_html_event);

  const std::vector<int> creative_ad_vote_registry =
      ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
          creative_ads, text_embedding_html_events);

  const std::vector<double> creative_ad_probabilities =
      ComputeCreativeAdProbabilitiesForVoteRegistry(creative_ad_vote_registry);
  CHECK_EQ(creative_ads.size(), creative_ad_probabilities.size());

  // Act
  const absl::optional<CreativeNotificationAdInfo> creative_ad =
      MaybeSampleCreativeAd(creative_ads, creative_ad_probabilities);

  // Assert
  EXPECT_TRUE(creative_ad);
}

TEST_F(BraveAdsCreativeAdEmbeddingBasedPredictorSamplingTest,
       DoNotSampleCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_1.embedding = {-0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad_1);

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  creative_ad_2.embedding = {0.0853, -0.1789, 0.4221};
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

  const TextEmbeddingHtmlEventList text_embedding_html_events;

  const std::vector<int> creative_ad_vote_registry =
      ComputeCreativeAdVoteRegistryForTextEmbeddingHtmlEvents(
          creative_ads, text_embedding_html_events);

  const std::vector<double> creative_ad_probabilities =
      ComputeCreativeAdProbabilitiesForVoteRegistry(creative_ad_vote_registry);
  CHECK_EQ(creative_ads.size(), creative_ad_probabilities.size());

  // Act
  const absl::optional<CreativeNotificationAdInfo> creative_ad =
      MaybeSampleCreativeAd(creative_ads, creative_ad_probabilities);

  // Assert
  EXPECT_FALSE(creative_ad);
}

}  // namespace brave_ads
