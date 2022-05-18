/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/time_since_last_user_activity_event.h"

#include <memory>

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/user_activity/browsing/user_activity.h"

// npm run test -- brave_unit_tests --filter=BatAdsFederatedLogEntries*

namespace ads {

class BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest
    : public UnitTestBase {
 protected:
  BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest() = default;

  ~BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest() override =
      default;
};

TEST_F(BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest,
       GetDataType) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry = std::make_unique<
      TimeSinceLastUserActivityEvent>(
      UserActivityEventType::kOpenedNewTab,
      brave_federated::mojom::CovariateType::kTimeSinceLastOpenedNewTabEvent);

  // Act
  brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kInt, data_type);
}

TEST_F(BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest,
       GetValueWithoutHistory) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry = std::make_unique<
      TimeSinceLastUserActivityEvent>(
      UserActivityEventType::kOpenedNewTab,
      brave_federated::mojom::CovariateType::kTimeSinceLastOpenedNewTabEvent);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("-1", value);
}

TEST_F(BatAdsFederatedLogEntriesTimeSinceLastUserActivityEventTest, GetValue) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry = std::make_unique<
      TimeSinceLastUserActivityEvent>(
      UserActivityEventType::kOpenedNewTab,
      brave_federated::mojom::CovariateType::kTimeSinceLastOpenedNewTabEvent);

  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  AdvanceClock(base::Minutes(2));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("120", value);
}

}  // namespace ads
