/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/embedding_based/creative_ad_embedding_based_predictor.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdEmbeddingBasedPredictorTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdEmbeddingBasedPredictorTest, PredictCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ad.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad);

  const TextEmbeddingHtmlEventInfo text_embedding_html_event =
      BuildTextEmbeddingHtmlEvent(ml::pipeline::test::BuildTextEmbedding());

  const UserModelInfo user_model{
      IntentUserModelInfo{}, LatentInterestUserModelInfo{},
      InterestUserModelInfo{/*segments=*/{}, TextEmbeddingHtmlEventList{
                                                 text_embedding_html_event}}};

  // Act & Assert
  EXPECT_TRUE(MaybePredictCreativeAd(creative_ads, user_model));
}

TEST_F(BraveAdsCreativeAdEmbeddingBasedPredictorTest, DoNotPredictCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;

  CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad.embedding = {0.0853, -0.1789, -0.4221};
  creative_ads.push_back(creative_ad);

  // Act & Assert
  EXPECT_FALSE(MaybePredictCreativeAd(creative_ads, /*user_model=*/{}));
}

}  // namespace brave_ads
