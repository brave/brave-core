/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/prediction/model_based/creative_ad_model_based_predictor_feature.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdModelBasedPredictorTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeAdModelBasedPredictorTest, PredictCreativeAd) {
  // Arrange
  CreativeNotificationAdList creative_ads;
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ads.push_back(creative_ad);

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(MaybePredictCreativeAd(creative_ads, user_model, ad_events));
}

TEST_F(BraveAdsCreativeAdModelBasedPredictorTest, DoNotPredictCreativeAd) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kCreativeAdModelBasedPredictorFeature,
      {{"child_intent_segment_ad_predictor_weight", "0.0"},
       {"parent_intent_segment_ad_predictor_weight", "0.0"},
       {"child_latent_interest_segment_ad_predictor_weight", "0.0"},
       {"parent_latent_interest_segment_ad_predictor_weight", "0.0"},
       {"child_interest_segment_ad_predictor_weight", "0.0"},
       {"parent_interest_segment_ad_predictor_weight", "0.0"},
       {"last_seen_ad_predictor_weight", "0.0"},
       {"last_seen_advertiser_ad_predictor_weight", "0.0"},
       {"priority_ad_predictor_weight", "0.0"}});

  CreativeNotificationAdList creative_ads;
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/
                                        true);
  creative_ads.push_back(creative_ad);

  const UserModelInfo user_model{
      IntentUserModelInfo{SegmentList{"parent-child"}},
      LatentInterestUserModelInfo{SegmentList{"parent-child"}},
      InterestUserModelInfo{SegmentList{"parent-child"},
                            TextEmbeddingHtmlEventList{}}};

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now(),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_FALSE(MaybePredictCreativeAd(creative_ads, user_model, ad_events));
}

}  // namespace brave_ads
