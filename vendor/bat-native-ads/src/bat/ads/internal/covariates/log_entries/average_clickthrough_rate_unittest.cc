/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"

#include <memory>

#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAverageClickthroughRateTest : public UnitTestBase {};

TEST_F(BatAdsAverageClickthroughRateTest, GetDataType) {
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(7));

  // Act
  const brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble, data_type);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValueInvalidClicks) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValueNotInTimeWindow) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  AdvanceClockBy(base::Days(2));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValueWithClickthroughRateOfZero) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValueWithClickthroughRateOfOne) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("1", value);
}

TEST_F(BatAdsAverageClickthroughRateTest, GetValue) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0.3333333333333333", value);
}

}  // namespace ads
