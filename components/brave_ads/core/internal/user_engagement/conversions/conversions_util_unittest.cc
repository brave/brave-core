/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsUtilTest : public test::TestBase {};

TEST_F(BraveAdsConversionsUtilTest, IsAllowedToConvertInlineContentAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertInlineContentAdEventIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsAllowedToConvertInlineContentAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, IsAllowedToConvertPromotedContentAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertPromotedContentAdEventIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsAllowedToConvertPromotedContentAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, IsAllowedToConvertNewTabPageAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertNewTabPageAdEventIfOptedOutOfNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertNewTabPageAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, IsAllowedToConvertNotificationAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertNotificationAdEventIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertNotificationAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, IsAllowedToConvertSearchResultAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertSearchResultAdEventIfOptedOutOfSearchResultAds) {
  // Arrange
  test::OptOutOfSearchResultAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(
    BraveAdsConversionsUtilTest,
    IsAllowedToConvertSearchResultAdEventForNonRewardsUserIfShouldAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kClicked,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest,
       IsNotAllowedToConvertSearchResultAdEventForNonRewardsUser) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kClicked,
                   /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsUtilTest, DidAdEventOccurWithinObservationWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(DidAdEventOccurWithinObservationWindow(
      ad_event, /*observation_window=*/base::Days(1)));
}

TEST_F(BraveAdsConversionsUtilTest, DidAdEventOccurOutsideObservationWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());

  AdvanceClockBy(base::Days(1) + base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(DidAdEventOccurWithinObservationWindow(
      ad_event, /*observation_window=*/base::Days(1)));
}

}  // namespace brave_ads
