/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/average_clickthrough_rate.h"

#include <memory>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/history/history.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesAverageClickthroughRateTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesAverageClickthroughRateTest() = default;

  ~BatAdsFederatedLogEntriesAverageClickthroughRateTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest, GetDataType) {
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(7));

  // Act
  brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble, data_type);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest,
       GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest,
       GetValueInvalidClicks) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest,
       GetValueNotInTimeWindow) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  AdvanceClock(base::Days(2));

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest,
       GetValueWithClickthroughRateOfZero) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest,
       GetValueWithClickthroughRateOfOne) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("1", value);
}

TEST_F(BatAdsFederatedLogEntriesAverageClickthroughRateTest, GetValue) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<AverageClickthroughRate>(base::Days(1));

  const AdNotificationInfo ad;
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0.3333333333333333", value);
}

}  // namespace ads
