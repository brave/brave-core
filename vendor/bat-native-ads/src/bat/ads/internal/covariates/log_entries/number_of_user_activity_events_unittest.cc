/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/number_of_user_activity_events.h"

#include <memory>

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest() = default;

  ~BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest() override = default;
};

TEST_F(BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest, GetDataType) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<NumberOfUserActivityEvents>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  // Act
  brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kInt, data_type);
}

TEST_F(BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest,
       GetValueWithoutUserActivity) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<NumberOfUserActivityEvents>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("0", value);
}

TEST_F(BatAdsFederatedLogEntriesNumberOfUserActivityEventsTest, GetValue) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry =
      std::make_unique<NumberOfUserActivityEvents>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kBrowserDidResignActive);

  AdvanceClockBy(base::Minutes(31));

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClosedTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("2", value);
}

}  // namespace ads
