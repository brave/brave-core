/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_handler_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdEventHandlerUtilTest, HasFiredAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed,
                                            /*created_at=*/Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed));
}

TEST(BraveAdsAdEventHandlerUtilTest, HasNeverFiredAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_FALSE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, WasAdServed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  const AdEventList ad_events;

  // Act & Assert
  EXPECT_TRUE(
      WasAdServed(ad, ad_events, mojom::InlineContentAdEventType::kServed));
}

TEST(BraveAdsAdEventHandlerUtilTest, WasAdPreviouslyServed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(
      WasAdServed(ad, ad_events, mojom::InlineContentAdEventType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, WasAdNeverServed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  const AdEventList ad_events;

  // Act & Assert
  EXPECT_FALSE(
      WasAdServed(ad, ad_events, mojom::InlineContentAdEventType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, ShouldDebouncePreviouslyViewedAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  AdEventInfo ad_event_1 = BuildAdEvent(ad, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_1);
  AdEventInfo ad_event_2 = BuildAdEvent(ad, ConfirmationType::kViewed, Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_TRUE(ShouldDebounceAdEvent(ad, ad_events,
                                    mojom::InlineContentAdEventType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, ShouldNotDebounceViewedAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  AdEventInfo ad_event = BuildAdEvent(ad, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_FALSE(ShouldDebounceAdEvent(ad, ad_events,
                                     mojom::InlineContentAdEventType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, ShouldDebouncePreviouslyClickedAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  AdEventInfo ad_event_1 = BuildAdEvent(ad, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_1);
  AdEventInfo ad_event_2 = BuildAdEvent(ad, ConfirmationType::kViewed, Now());
  ad_events.push_back(ad_event_2);
  AdEventInfo ad_event_3 = BuildAdEvent(ad, ConfirmationType::kClicked, Now());
  ad_events.push_back(ad_event_3);

  // Act & Assert
  EXPECT_TRUE(ShouldDebounceAdEvent(ad, ad_events,
                                    mojom::InlineContentAdEventType::kClicked));
}

TEST(BraveAdsAdEventHandlerUtilTest, ShouldNotDebounceClickedAdEvent) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);

  AdEventList ad_events;
  AdEventInfo ad_event_1 = BuildAdEvent(ad, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event_1);
  AdEventInfo ad_event_2 = BuildAdEvent(ad, ConfirmationType::kViewed, Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(ShouldDebounceAdEvent(
      ad, ad_events, mojom::InlineContentAdEventType::kClicked));
}

}  // namespace brave_ads
