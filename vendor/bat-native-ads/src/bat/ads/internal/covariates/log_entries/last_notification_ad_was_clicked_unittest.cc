/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/last_notification_ad_was_clicked.h"

#include <memory>

#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsLastNotificationAdWasClickedTest : public UnitTestBase {};

TEST_F(BatAdsLastNotificationAdWasClickedTest, GetDataType) {
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  // Act
  const brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kBool, data_type);
}

TEST_F(BatAdsLastNotificationAdWasClickedTest, GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsLastNotificationAdWasClickedTest, GetValueNotInTimeWindow) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  AdvanceClockBy(base::Days(31));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsLastNotificationAdWasClickedTest, GetValueWasClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("1", value);
}

TEST_F(BatAdsLastNotificationAdWasClickedTest, GetValueWasNotClicked) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<LastNotificationAdWasClicked>();

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

}  // namespace ads
