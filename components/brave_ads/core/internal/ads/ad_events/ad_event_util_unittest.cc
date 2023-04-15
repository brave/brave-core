/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_util.h"

#include <string>

#include "base/guid.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForEmptyAdEvents) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(GetLastSeenAdTime(/*ad_events*/ {}, creative_ad));
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForUnseenAd) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  const base::Time event_time = Now() - base::Hours(12);
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, event_time);
  ad_events.push_back(ad_event);

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(GetLastSeenAdTime(ad_events, creative_ad_2));
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTime) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  const base::Time now = Now();

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kConversion, now - base::Hours(3));
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(6));
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(11));
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(12));
  ad_events.push_back(ad_event_1);

  // Act
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad_1);
  ASSERT_TRUE(last_seen_ad_time);

  // Assert
  EXPECT_EQ(now - base::Hours(6), *last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForEmptyAdEvents) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(GetLastSeenAdvertiserTime(/*ad_events*/ {}, creative_ad));
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForUnseenAdvertiser) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, Now() - base::Hours(12));
  ad_events.push_back(ad_event);

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(GetLastSeenAdvertiserTime(ad_events, creative_ad_2));
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTime) {
  // Arrange
  const std::string advertiser_id_1 =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  const std::string advertiser_id_2 =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_1.advertiser_id = advertiser_id_1;

  CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_2.advertiser_id = advertiser_id_2;

  CreativeNotificationAdInfo creative_ad_3 =
      BuildCreativeNotificationAd(/*should_use_random_guids*/ true);
  creative_ad_3.advertiser_id = advertiser_id_1;

  AdEventList ad_events;

  const base::Time now = Now();

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(3));
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad_3, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(6));
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad_2, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(11));
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, now - base::Hours(12));
  ad_events.push_back(ad_event_1);

  // Act
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad_3);
  ASSERT_TRUE(last_seen_advertiser_time);

  // Assert
  EXPECT_EQ(now - base::Hours(3), *last_seen_advertiser_time);
}

}  // namespace brave_ads
