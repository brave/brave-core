/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_util.h"

#include <string>

#include "base/guid.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForEmptyAdEvents) {
  // Arrange
  AdEventList ad_events;

  const CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();

  // Act
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTimeForUnseenAd) {
  // Arrange
  AdEventList ad_events;

  const CreativeAdNotificationInfo creative_ad_1 =
      BuildCreativeAdNotification();

  const base::Time event_time =
      base::Time::Now() - base::TimeDelta::FromHours(12);
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed, event_time);
  ad_events.push_back(ad_event);

  // Act
  const CreativeAdNotificationInfo creative_ad_2 =
      BuildCreativeAdNotification();
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad_2);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_ad_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdTime) {
  // Arrange
  AdEventList ad_events;

  const base::Time now = base::Time::Now();

  const CreativeAdNotificationInfo creative_ad_1 =
      BuildCreativeAdNotification();

  const CreativeAdNotificationInfo creative_ad_2 =
      BuildCreativeAdNotification();

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad_1, ConfirmationType::kConversion,
                   now - base::TimeDelta::FromHours(3));
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(6));
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad_2, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(11));
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(12));
  ad_events.push_back(ad_event_1);

  // Act
  const absl::optional<base::Time> last_seen_ad_time =
      GetLastSeenAdTime(ad_events, creative_ad_1);

  // Assert
  const base::Time expected_last_seen_ad_time =
      now - base::TimeDelta::FromHours(6);
  EXPECT_EQ(expected_last_seen_ad_time, last_seen_ad_time.value());
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForEmptyAdEvents) {
  // Arrange
  AdEventList ad_events;

  const CreativeAdNotificationInfo creative_ad = BuildCreativeAdNotification();

  // Act
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_advertiser_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTimeForUnseenAdvertiser) {
  // Arrange
  AdEventList ad_events;

  const CreativeAdNotificationInfo creative_ad_1 =
      BuildCreativeAdNotification();

  const base::Time event_time =
      base::Time::Now() - base::TimeDelta::FromHours(12);
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed, event_time);
  ad_events.push_back(ad_event);

  // Act
  const CreativeAdNotificationInfo creative_ad_2 =
      BuildCreativeAdNotification();
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad_2);

  // Assert
  EXPECT_EQ(absl::nullopt, last_seen_advertiser_time);
}

TEST(BatAdsAdEventUtilTest, GetLastSeenAdvertiserTime) {
  // Arrange
  const std::string advertiser_1 = base::GenerateGUID();
  const std::string advertiser_2 = base::GenerateGUID();

  CreativeAdNotificationInfo creative_ad_1 = BuildCreativeAdNotification();
  creative_ad_1.advertiser_id = advertiser_1;

  CreativeAdNotificationInfo creative_ad_2 = BuildCreativeAdNotification();
  creative_ad_2.advertiser_id = advertiser_2;

  CreativeAdNotificationInfo creative_ad_3 = BuildCreativeAdNotification();
  creative_ad_3.advertiser_id = advertiser_1;

  AdEventList ad_events;

  const base::Time now = base::Time::Now();

  const AdEventInfo ad_event_4 =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(3));
  ad_events.push_back(ad_event_4);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(creative_ad_3, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(6));
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad_2, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(11));
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad_1, ConfirmationType::kViewed,
                   now - base::TimeDelta::FromHours(12));
  ad_events.push_back(ad_event_1);

  // Act
  const absl::optional<base::Time> last_seen_advertiser_time =
      GetLastSeenAdvertiserTime(ad_events, creative_ad_3);

  // Assert
  const base::Time expected_last_seen_advertiser_time =
      now - base::TimeDelta::FromHours(3);
  EXPECT_EQ(expected_last_seen_advertiser_time,
            last_seen_advertiser_time.value());
}

}  // namespace ads
