/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/covariate_logs.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsCovariateLogsTest*

namespace ads {

class BatAdsCovariateLogsTest : public UnitTestBase {
 protected:
  BatAdsCovariateLogsTest() = default;

  ~BatAdsCovariateLogsTest() override = default;
};

TEST_F(BatAdsCovariateLogsTest, GetTrainingInstance) {
  // Arrange

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      CovariateLogs::Get()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(30U, training_covariates->covariates.size());
}

TEST_F(BatAdsCovariateLogsTest, GetTrainingInstanceWithSetters) {
  // Arrange
  CovariateLogs::Get()->SetAdNotificationServedAt(Now());
  CovariateLogs::Get()->SetAdNotificationClicked(true);

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      CovariateLogs::Get()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(32U, training_covariates->covariates.size());
}

}  // namespace ads
