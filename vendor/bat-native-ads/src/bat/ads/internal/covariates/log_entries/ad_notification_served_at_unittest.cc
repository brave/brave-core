/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/ad_notification_served_at.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesAdNotificationServedAtTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesAdNotificationServedAtTest() = default;

  ~BatAdsFederatedLogEntriesAdNotificationServedAtTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesAdNotificationServedAtTest, GetDataType) {
  AdNotificationServedAt ad_notification_served_at;

  // Act
  brave_federated::mojom::DataType data_type =
      ad_notification_served_at.GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble, data_type);
}

TEST_F(BatAdsFederatedLogEntriesAdNotificationServedAtTest, GetValue) {
  // Arrange
  AdNotificationServedAt ad_notification_served_at;

  // Act
  const base::Time now = TimeFromString("August 19 2019", /* is_local */ false);
  ad_notification_served_at.SetTime(now);
  const std::string value = ad_notification_served_at.GetValue();

  // Assert
  // Monday, 19 August 2019 00:00:00
  const std::string expected_value = "1566172800";
  EXPECT_EQ(expected_value, value);
}

TEST_F(BatAdsFederatedLogEntriesAdNotificationServedAtTest,
       GetValueWithoutTime) {
  // Arrange
  AdNotificationServedAt ad_notification_served_at;

  // Act
  const std::string value = ad_notification_served_at.GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

}  // namespace ads
