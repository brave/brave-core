/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdEventHandlerUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdEventHandlerUtilTest, HasFiredAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kServedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, HasNeverFiredAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_FALSE(
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, HasFiredAdEventWithinTimeWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(HasFiredAdEventWithinTimeWindow(
      ad, ad_events, ConfirmationType::kServedImpression,
      /*time_window=*/base::Seconds(5)));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, HasNeverFiredAdEventWithinTimeWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  AdvanceClockBy(base::Seconds(5));

  // Act & Assert
  EXPECT_FALSE(HasFiredAdEventWithinTimeWindow(
      ad, ad_events, ConfirmationType::kViewedImpression,
      /*time_window=*/base::Seconds(5)));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, WasAdServed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(WasAdServed(ad, ad_events,
                          mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, WasAdServedIfNoPreviousEvents) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(WasAdServed(ad, /*ad_events=*/{},
                          mojom::InlineContentAdEventType::kServedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, WasAdNeverServed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(WasAdServed(ad, /*ad_events=*/{},
                           mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldDeduplicateViewedAdEventWithinTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_viewed_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  AdvanceClockBy(kDeduplicateViewedAdEventFor.Get());

  // Act & Assert
  EXPECT_TRUE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldNotDeduplicateViewedAdEventOutOfTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_viewed_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  AdvanceClockBy(kDeduplicateViewedAdEventFor.Get() + base::Seconds(1));

  // Act & Assert
  EXPECT_FALSE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, ShouldAlwaysDeduplicateViewedAdEvent) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_viewed_ad_event_for", "0s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  AdvanceClockTo(test::DistantFuture());

  // Act & Assert
  EXPECT_TRUE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldNotDeduplicateViewedAdEventIfAdWasNeverViewed) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_viewed_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_FALSE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kViewedImpression));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldDeduplicateClickedAdEventWithinTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_clicked_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_3);

  AdvanceClockBy(kDeduplicateClickedAdEventFor.Get());

  // Act & Assert
  EXPECT_TRUE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kClicked));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldNotDeduplicateClickedAdEventOutOfTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_clicked_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_3);

  AdvanceClockBy(kDeduplicateClickedAdEventFor.Get() + base::Seconds(1));

  // Act & Assert
  EXPECT_FALSE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kClicked));
}

TEST_F(BraveAdsAdEventHandlerUtilTest, ShouldAlwaysDeduplicateClickedAdEvent) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_clicked_ad_event_for", "0s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_3);

  AdvanceClockTo(test::DistantFuture());

  // Act & Assert
  EXPECT_TRUE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kClicked));
}

TEST_F(BraveAdsAdEventHandlerUtilTest,
       ShouldNotDeduplicateClickedAdEventIfAdWasNeverClicked) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_clicked_ad_event_for", "5s"}});

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(ShouldDeduplicateAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kClicked));
}

}  // namespace brave_ads
