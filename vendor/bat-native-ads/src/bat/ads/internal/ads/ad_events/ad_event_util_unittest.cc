/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/ad_event_util.h"

#include <string>

#include "base/guid.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForEmptyAdEvents) {
  // Arrange
  const AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();

  // Act
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForUnseenAd) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd();

  const base::Time event_time = Now() - base::Hours(12);
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, event_time);
  ad_events.push_back(ad_event);

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd();
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad_2);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTime) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd();

  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd();

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

  // Assert
  const base::Time expected_last_seen_ad_time = now - base::Hours(6);
  EXPECT_EQ(expected_last_seen_ad_time, *last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForEmptyAdEvents) {
  // Arrange
  const AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();

  // Act
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_advertiser_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForUnseenAdvertiser) {
  // Arrange
  AdEventList ad_events;

  const CreativeNotificationAdInfo creative_ad_1 =
      BuildCreativeNotificationAd();

  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, AdType::kNotificationAd,
                   ConfirmationType::kViewed, Now() - base::Hours(12));
  ad_events.push_back(ad_event);

  // Act
  const CreativeNotificationAdInfo creative_ad_2 =
      BuildCreativeNotificationAd();
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad_2);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_advertiser_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTime) {
  // Arrange
  const std::string advertiser_1 =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  const std::string advertiser_2 =
      base::GUID::GenerateRandomV4().AsLowercaseString();

  CreativeNotificationAdInfo creative_ad_1 = BuildCreativeNotificationAd();
  creative_ad_1.advertiser_id = advertiser_1;

  CreativeNotificationAdInfo creative_ad_2 = BuildCreativeNotificationAd();
  creative_ad_2.advertiser_id = advertiser_2;

  CreativeNotificationAdInfo creative_ad_3 = BuildCreativeNotificationAd();
  creative_ad_3.advertiser_id = advertiser_1;

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

  // Assert
  const base::Time expected_last_seen_advertiser_time = now - base::Hours(3);
  EXPECT_EQ(expected_last_seen_advertiser_time, *last_seen_advertiser_time);
}

}  // namespace ads
