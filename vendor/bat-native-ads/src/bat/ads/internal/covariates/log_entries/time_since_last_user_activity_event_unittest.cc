/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/time_since_last_user_activity_event.h"

#include <memory>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTimeSinceLastUserActivityEventTest : public UnitTestBase {};

TEST_F(BatAdsTimeSinceLastUserActivityEventTest, GetDataType) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry = std::make_unique<
      TimeSinceLastUserActivityEvent>(
      UserActivityEventType::kOpenedNewTab,
      brave_federated::mojom::CovariateType::kTimeSinceLastOpenedNewTabEvent);

  // Act
  const brave_federated::mojom::DataType data_type = entry->GetDataType();

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kInt, data_type);
}

TEST_F(BatAdsTimeSinceLastUserActivityEventTest, GetValueWithoutHistory) {
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

TEST_F(BatAdsTimeSinceLastUserActivityEventTest, GetValue) {
  // Arrange
  std::unique_ptr<CovariateLogEntryInterface> entry = std::make_unique<
      TimeSinceLastUserActivityEvent>(
      UserActivityEventType::kOpenedNewTab,
      brave_federated::mojom::CovariateType::kTimeSinceLastOpenedNewTabEvent);

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  AdvanceClockBy(base::Minutes(2));

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClosedTab);

  // Act
  const std::string value = entry->GetValue();

  // Assert
  EXPECT_EQ("120", value);
}

}  // namespace ads
