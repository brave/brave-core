/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_clicked.h"

#include "bat/ads/internal/base/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesNotificationAdClickedTest : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesNotificationAdClickedTest() = default;

  ~BatAdsFederatedLogEntriesNotificationAdClickedTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesNotificationAdClickedTest, GetDataType) {
  NotificationAdClicked notification_ad_clicked;

  // Act
  brave_federated::mojom::DataType data_type =
      notification_ad_clicked.GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsFederatedLogEntriesNotificationAdClickedTest, GetValue) {
  // Arrange
  NotificationAdClicked notification_ad_clicked;

  // Act
  notification_ad_clicked.SetClicked(true);
  const std::string value = notification_ad_clicked.GetValue();

  // Assert
  EXPECT_EQ("true", value);
}

TEST_F(BatAdsFederatedLogEntriesNotificationAdClickedTest,
       GetValueWithoutSetClicked) {
  // Arrange
  NotificationAdClicked notification_ad_clicked;

  // Act
  const std::string value = notification_ad_clicked.GetValue();

  // Assert
  EXPECT_EQ("false", value);
}

}  // namespace ads
