/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariate_logs.h"

#include "base/time/time.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsCovariateLogsTest*

namespace ads {

class BatAdsCovariateLogsTest : public UnitTestBase {
 protected:
  BatAdsCovariateLogsTest() = default;

  ~BatAdsCovariateLogsTest() override = default;
};

TEST_F(BatAdsCovariateLogsTest, GetTrainingCovariates) {
  // Arrange

  // Act
  brave_federated::mojom::TrainingCovariatesPtr training_covariates =
      CovariateLogs::Get()->GetTrainingCovariates();

  // Assert
  EXPECT_EQ(34U, training_covariates->covariates.size());
}

TEST_F(BatAdsCovariateLogsTest, GetTrainingCovariatesWithSetters) {
  // Arrange
  CovariateLogs::Get()->SetAdNotificationImpressionServedAt(Now());
  CovariateLogs::Get()->SetAdNotificationWasClicked(true);

  // Act
  brave_federated::mojom::TrainingCovariatesPtr training_covariates =
      CovariateLogs::Get()->GetTrainingCovariates();

  // Assert
  EXPECT_EQ(36U, training_covariates->covariates.size());
}

}  // namespace ads
