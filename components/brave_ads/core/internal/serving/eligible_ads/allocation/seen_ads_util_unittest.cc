/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_ads_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSeenAdsUtilTest, DoNotGetLastSeenAdAtForEmptyAdEvents) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(GetLastSeenAdAt(/*ad_events=*/{}, creative_ad));
}

TEST(BraveAdsSeenAdsUtilTest, DoNotGetLastSeenAdAtForUnseenAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad_1, AdType::kNotificationAd, ConfirmationType::kViewed,
      /*created_at=*/Now() - base::Hours(12),
      /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(GetLastSeenAdAt(ad_events, creative_ad_2));
}

TEST(BraveAdsSeenAdsUtilTest, GetLastSeenAdAt) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  const CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  const base::Time now = Now();

  const AdEventInfo ad_event_4 = test::BuildAdEvent(
      creative_ad_1, AdType::kNotificationAd, ConfirmationType::kConversion,
      now - base::Hours(3), /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      test::BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                         ConfirmationType::kViewed, now - base::Hours(6),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_2 =
      test::BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                         ConfirmationType::kViewed, now - base::Hours(11),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_1 =
      test::BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                         ConfirmationType::kViewed, now - base::Hours(12),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_1);

  // Act & Assert
  EXPECT_EQ(now - base::Hours(6), GetLastSeenAdAt(ad_events, creative_ad_1));
}

}  // namespace brave_ads
