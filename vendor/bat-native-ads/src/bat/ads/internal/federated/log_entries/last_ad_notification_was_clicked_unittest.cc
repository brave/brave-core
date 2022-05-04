/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/last_ad_notification_was_clicked.h"

#include <memory>

#include "base/time/time.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/history/history.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest() = default;

  ~BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest() override =
      default;
};

TEST_F(BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest, GetDataType) {
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastAdNotificationWasClicked>();

  // Act
  brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest,
       GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastAdNotificationWasClicked>();

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest,
       GetValueNotInTimeWindow) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastAdNotificationWasClicked>();

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  AdvanceClock(base::Days(31));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest,
       GetValueWasClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastAdNotificationWasClicked>();

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastAdNotificationWasClickedTest,
       GetValueWasNotClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastAdNotificationWasClicked>();

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kClicked);
  history::AddAdNotification(ad, ConfirmationType::kViewed);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

}  // namespace ads
