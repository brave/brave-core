/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_maintenance.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database {

class BraveAdsDatabaseMaintenanceTest : public test::TestBase {
 protected:
  void SetUpMocks() override {
    // Must be created here so it registers with DatabaseManager before
    // OnDatabaseIsReady fires during TestBase::SetUp.
    maintenance_ = std::make_unique<Maintenance>();
  }

  std::unique_ptr<Maintenance> maintenance_;
};

TEST_F(BraveAdsDatabaseMaintenanceTest, SchedulesInitialMaintenance) {
  // Assert
  EXPECT_EQ(base::Minutes(1), NextPendingTaskDelay());
}

TEST_F(BraveAdsDatabaseMaintenanceTest, ReschedulesAfterRecurringInterval) {
  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_EQ(base::Days(1), NextPendingTaskDelay());
}

}  // namespace brave_ads::database
