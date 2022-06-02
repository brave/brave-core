/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/last_notification_ad_was_clicked.h"

#include <memory>

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/history/history.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest() = default;

  ~BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest() override =
      default;
};

TEST_F(BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest, GetDataType) {
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  // Act
  brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest,
       GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest,
       GetValueNotInTimeWindow) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  history::AddNotificationAd(ad, ConfirmationType::kViewed);
  history::AddNotificationAd(ad, ConfirmationType::kClicked);

  AdvanceClock(base::Days(31));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest,
       GetValueWasClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  history::AddNotificationAd(ad, ConfirmationType::kViewed);
  history::AddNotificationAd(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("1", value);
}

TEST_F(BatAdsFederatedLogEntriesLastNotificationAdWasClickedTest,
       GetValueWasNotClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  history::AddNotificationAd(ad, ConfirmationType::kClicked);
  history::AddNotificationAd(ad, ConfirmationType::kViewed);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

}  // namespace ads
