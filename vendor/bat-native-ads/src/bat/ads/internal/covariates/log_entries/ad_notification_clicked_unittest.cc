/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/ad_notification_clicked.h"

#include "bat/ads/internal/base/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesAdNotificationClickedTest : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesAdNotificationClickedTest() = default;

  ~BatAdsFederatedLogEntriesAdNotificationClickedTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesAdNotificationClickedTest, GetDataType) {
  AdNotificationClicked ad_notification_clicked;

  // Act
  brave_federated::mojom::DataType data_type =
      ad_notification_clicked.GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsFederatedLogEntriesAdNotificationClickedTest, GetValue) {
  // Arrange
  AdNotificationClicked ad_notification_clicked;

  // Act
  ad_notification_clicked.SetClicked(true);
  const std::string value = ad_notification_clicked.GetValue();

  // Assert
  EXPECT_EQ("true", value);
}

TEST_F(BatAdsFederatedLogEntriesAdNotificationClickedTest,
       GetValueWithoutSetClicked) {
  // Arrange
  AdNotificationClicked ad_notification_clicked;

  // Act
  const std::string value = ad_notification_clicked.GetValue();

  // Assert
  EXPECT_EQ("false", value);
}

}  // namespace ads
