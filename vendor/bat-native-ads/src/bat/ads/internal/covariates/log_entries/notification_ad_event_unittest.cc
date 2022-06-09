/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesNotificationAdEventTest : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesNotificationAdEventTest() = default;

  ~BatAdsFederatedLogEntriesNotificationAdEventTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesNotificationAdEventTest, GetDataType) {
  NotificationAdEvent notification_ad_event;

  // Act
  brave_federated::mojom::DataType data_type =
      notification_ad_event.GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsFederatedLogEntriesNotificationAdEventTest, GetValue) {
  // Arrange
  NotificationAdEvent notification_ad_event;

  // Act
  notification_ad_event.SetEventType(mojom::NotificationAdEventType::kClicked);
  const std::string value = notification_ad_event.GetValue();

  // Assert
  EXPECT_EQ("clicked", value);
}

TEST_F(BatAdsFederatedLogEntriesNotificationAdEventTest,
       GetValueWhenDismissed) {
  // Arrange
  NotificationAdEvent notification_ad_event;

  // Act
  notification_ad_event.SetEventType(
      mojom::NotificationAdEventType::kDismissed);
  const std::string value = notification_ad_event.GetValue();

  // Assert
  EXPECT_EQ("dismissed", value);
}

TEST_F(BatAdsFederatedLogEntriesNotificationAdEventTest,
       GetValueWithoutSetInteraction) {
  // Arrange
  NotificationAdEvent notification_ad_event;

  // Act
  const std::string value = notification_ad_event.GetValue();

  // Assert
  EXPECT_EQ("timedOut", value);
}

}  // namespace ads
