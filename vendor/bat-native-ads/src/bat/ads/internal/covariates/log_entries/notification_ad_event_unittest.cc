/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"

#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetValueForEventType(
    const mojom::NotificationAdEventType event_type) {
  NotificationAdEvent notification_ad_event;
  notification_ad_event.SetEventType(event_type);
  return notification_ad_event.GetValue();
}

std::string GetEventTypeAsString(
    const mojom::NotificationAdEventType event_type) {
  std::stringstream ss;
  ss << event_type;
  return ss.str();
}

}  // namespace

TEST(BatAdsNotificationAdEventTest, GetDataType) {
  const NotificationAdEvent notification_ad_event;

  // Act
  const brave_federated::mojom::DataType data_type =
      notification_ad_event.GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kString, data_type);
}

TEST(BatAdsNotificationAdEventTest, GetValueWhenClicked) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kClicked;

  // Act
  const std::string value = GetValueForEventType(event_type);

  // Assert
  const std::string expected_value = GetEventTypeAsString(event_type);
  EXPECT_EQ(expected_value, value);
}

TEST(BatAdsNotificationAdEventTest, GetValueWhenDismissed) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kDismissed;

  // Act
  const std::string value = GetValueForEventType(event_type);

  // Assert
  const std::string expected_value = GetEventTypeAsString(event_type);
  EXPECT_EQ(expected_value, value);
}

TEST(BatAdsNotificationAdEventTest, GetValueWhenTimeout) {
  // Arrange
  const mojom::NotificationAdEventType event_type =
      mojom::NotificationAdEventType::kTimedOut;

  // Act
  const std::string value = GetValueForEventType(event_type);

  // Assert
  const std::string expected_value = GetEventTypeAsString(event_type);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
