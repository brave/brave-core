/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/covariate_manager.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsCovariateManagerTest*

namespace ads {

class BatAdsCovariateManagerTest : public UnitTestBase {
 protected:
  BatAdsCovariateManagerTest() = default;

  ~BatAdsCovariateManagerTest() override = default;
};

TEST_F(BatAdsCovariateManagerTest, GetTrainingInstance) {
  // Arrange

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      CovariateManager::Get()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(30U, training_covariates->covariates.size());
}

TEST_F(BatAdsCovariateManagerTest, GetTrainingInstanceWithSetters) {
  // Arrange
  CovariateManager::Get()->SetNotificationAdServedAt(Now());
  CovariateManager::Get()->SetNotificationAdClicked(true);

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      CovariateManager::Get()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(32U, training_covariates->covariates.size());
}

}  // namespace ads
