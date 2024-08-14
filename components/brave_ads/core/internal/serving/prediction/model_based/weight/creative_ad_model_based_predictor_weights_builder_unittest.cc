/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/weight/creative_ad_model_based_predictor_weights_builder.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorWeightsBuilderTest
    : public test::TestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorWeightsBuilderTest,
       BuildCreativeInlineContentAdModelBasedPredictorWeights) {
  // Arrange
  const CreativeInlineContentAdList creative_ads =
      test::BuildCreativeInlineContentAds(/*count=*/1);

  // Act
  const CreativeAdModelBasedPredictorWeightsInfo weights =
      BuildCreativeAdModelBasedPredictorWeights(creative_ads);

  // Assert
  CreativeAdModelBasedPredictorWeightsInfo expected_weights;
  expected_weights.intent_segment.child = 0.0;
  expected_weights.intent_segment.parent = 0.0;
  expected_weights.latent_interest_segment.child = 0.0;
  expected_weights.latent_interest_segment.parent = 0.0;
  expected_weights.interest_segment.child = 0.0;
  expected_weights.interest_segment.parent = 0.0;
  expected_weights.untargeted_segment = 0.0001;
  expected_weights.last_seen_ad = 0.0;
  EXPECT_EQ(expected_weights, weights);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorWeightsBuilderTest,
       BuildCreativeNewTabPageAdModelBasedPredictorWeights) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      test::BuildCreativeNewTabPageAds(/*count=*/1);

  // Act
  const CreativeAdModelBasedPredictorWeightsInfo weights =
      BuildCreativeAdModelBasedPredictorWeights(creative_ads);

  // Assert
  CreativeAdModelBasedPredictorWeightsInfo expected_weights;
  expected_weights.intent_segment.child = 0.0;
  expected_weights.intent_segment.parent = 0.0;
  expected_weights.latent_interest_segment.child = 0.0;
  expected_weights.latent_interest_segment.parent = 0.0;
  expected_weights.interest_segment.child = 0.0;
  expected_weights.interest_segment.parent = 0.0;
  expected_weights.untargeted_segment = 0.0001;
  expected_weights.last_seen_ad = 0.0;
  EXPECT_EQ(expected_weights, weights);
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorWeightsBuilderTest,
       BuildCreativeNotificationAdModelBasedPredictorWeights) {
  // Arrange
  const CreativeNotificationAdList creative_ads =
      test::BuildCreativeNotificationAds(/*count=*/1);

  // Act
  const CreativeAdModelBasedPredictorWeightsInfo weights =
      BuildCreativeAdModelBasedPredictorWeights(creative_ads);

  // Assert
  CreativeAdModelBasedPredictorWeightsInfo expected_weights;
  expected_weights.intent_segment.child = 1.0;
  expected_weights.intent_segment.parent = 1.0;
  expected_weights.latent_interest_segment.child = 1.0;
  expected_weights.latent_interest_segment.parent = 1.0;
  expected_weights.interest_segment.child = 1.0;
  expected_weights.interest_segment.parent = 1.0;
  expected_weights.untargeted_segment = 0.0001;
  expected_weights.last_seen_ad = 0.0;
  EXPECT_EQ(expected_weights, weights);
}

}  // namespace brave_ads
