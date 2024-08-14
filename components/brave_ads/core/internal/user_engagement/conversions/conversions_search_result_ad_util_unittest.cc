/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsSearchResultAdUtilTest : public test::TestBase {};

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       AllowedToConvertViewedAdEvent) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(AdType::kSearchResultAd, /*should_generate_random_uuids=*/
                    true);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(
    BraveAdsConversionsSearchResultAdUtilTest,
    NotAllowedToConvertViewedAdEventForNonRewardsUserIfShouldNotAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       NotAllowedToConvertViewedAdEventIfOptedOutOfSearchResultAds) {
  // Arrange
  test::OptOutOfSearchResultAds();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       AllowedToConvertAdClickedEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       NotAllowedToConvertAdClickedEventIfOptedOutOfSearchResultAds) {
  // Arrange
  test::OptOutOfSearchResultAds();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       NotAllowedToConvertAdNonViewedOrClickedEvents) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);

  // Act & Assert
  for (int i = 0; i < static_cast<int>(ConfirmationType::kMaxValue); ++i) {
    const auto confirmation_type = static_cast<ConfirmationType>(i);
    if (confirmation_type == ConfirmationType::kViewedImpression ||
        confirmation_type == ConfirmationType::kClicked) {
      continue;
    }

    const AdEventInfo ad_event =
        BuildAdEvent(ad, confirmation_type, /*created_at=*/test::Now());
    EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
  }
}

TEST_F(BraveAdsConversionsSearchResultAdUtilTest,
       NotAllowedToConvertViewedAdEventForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(
    BraveAdsConversionsSearchResultAdUtilTest,
    NotAllowedToConvertAdClickedEventForNonRewardsUserIfShouldNotAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_FALSE(IsAllowedToConvertAdEvent(ad_event));
}

TEST_F(
    BraveAdsConversionsSearchResultAdUtilTest,
    AllowedToConvertAdClickedEventForNonRewardsUserIfShouldAlwaysTriggerBraveSearchResultAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(IsAllowedToConvertAdEvent(ad_event));
}

}  // namespace brave_ads
