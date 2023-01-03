/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/covariate_manager.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCovariateManagerTest : public UnitTestBase {};

TEST_F(BatAdsCovariateManagerTest, GetTrainingInstance) {
  // Arrange

  // Act
  const std::vector<brave_federated::mojom::CovariateInfoPtr>
      training_covariates =
          CovariateManager::GetInstance()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(32U, training_covariates.size());
}

TEST_F(BatAdsCovariateManagerTest, GetTrainingInstanceWithSetters) {
  // Arrange
  CovariateManager::GetInstance()->SetNotificationAdServedAt(Now());
  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kClicked);

  // Act
  const std::vector<brave_federated::mojom::CovariateInfoPtr>
      training_covariates =
          CovariateManager::GetInstance()->GetTrainingInstance();

  // Assert
  EXPECT_EQ(34U, training_covariates.size());
}

}  // namespace ads
