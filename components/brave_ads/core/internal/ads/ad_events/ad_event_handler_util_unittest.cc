/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_handler_util.h"

#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdEventHandlerUtilTest, HasFiredAdEvent) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  ad_events.push_back(ad_event);

  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, ad_event.placement_id);

  // Act

  // Assert
  EXPECT_TRUE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

TEST(BraveAdsAdEventHandlerUtilTest, HasNotFiredAdEvent) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);

  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kServed, Now());
  ad_events.push_back(ad_event);

  const NotificationAdInfo ad =
      BuildNotificationAd(creative_ad, ad_event.placement_id);

  // Act

  // Assert
  EXPECT_FALSE(HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

}  // namespace brave_ads
