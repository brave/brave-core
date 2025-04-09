/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_util.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest
    : public test::TestBase {};

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       IsAllowedToFireAdEvent) {
  // Act & Assert
  EXPECT_TRUE(IsAllowedToFireAdEvent());
}

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       IsNotAllowedToFireAdEvent) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_FALSE(IsAllowedToFireAdEvent());
}

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       ShouldFireNonDuplicateViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                              /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(ShouldFireAdEvent(
      ad, ad_events, mojom::NewTabPageAdEventType::kViewedImpression));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       ShouldNotFireDuplicateViewedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                              /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(ShouldFireAdEvent(
      ad, ad_events, mojom::NewTabPageAdEventType::kViewedImpression));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       ShouldFireNonDuplicateClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                              /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(
      ShouldFireAdEvent(ad, ad_events, mojom::NewTabPageAdEventType::kClicked));
}

TEST_F(BraveAdsNewTabPageAdEventHandlerUtilForRewardsTest,
       ShouldNotFireDuplicateClickedEvent) {
  // Arrange
  const NewTabPageAdInfo ad =
      test::BuildNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                              /*should_generate_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, mojom::ConfirmationType::kClicked, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(
      ShouldFireAdEvent(ad, ad_events, mojom::NewTabPageAdEventType::kClicked));
}

}  // namespace brave_ads
