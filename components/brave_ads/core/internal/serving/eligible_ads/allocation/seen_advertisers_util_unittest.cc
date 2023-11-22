/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_advertisers_util.h"

#include <string>

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSeenAdvertisersUtilTest,
     DoNotGetLastSeenAdvertiserAtForEmptyAdEvents) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(GetLastSeenAdvertiserAt(/*ad_events=*/{}, creative_ad));
}

TEST(BraveAdsSeenAdvertisersUtilTest,
     DoNotGetLastSeenAdvertiserAtForUnseenAdvertiser) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      test::BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                         ConfirmationType::kViewed, Now() - base::Hours(12),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event);

  const CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(GetLastSeenAdvertiserAt(ad_events, creative_ad_2));
}

TEST(BraveAdsSeenAdvertisersUtilTest, GetLastSeenAdvertiserAt) {
  // Arrange
  const std::string advertiser_id_1 =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  const std::string advertiser_id_2 =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  CreativeNotificationAdInfo creative_ad_1 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_1.advertiser_id = advertiser_id_1;

  CreativeNotificationAdInfo creative_ad_2 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_2.advertiser_id = advertiser_id_2;

  CreativeNotificationAdInfo creative_ad_3 =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  creative_ad_3.advertiser_id = advertiser_id_1;

  const base::Time now = Now();

  AdEventList ad_events;

  const AdEventInfo ad_event_4 =
      test::BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                         ConfirmationType::kViewed, now - base::Hours(3),
                         /*should_use_random_uuids=*/true);
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      test::BuildAdEvent(creative_ad_3, AdType::kNotificationAd,
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
  EXPECT_EQ(now - base::Hours(3),
            GetLastSeenAdvertiserAt(ad_events, creative_ad_3));
}

}  // namespace brave_ads
