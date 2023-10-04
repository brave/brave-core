/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPredictorsManagerTest : public UnitTestBase {};

TEST_F(BraveAdsPredictorsManagerTest, GetTrainingSample) {
  // Act
  const std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample =
      PredictorsManager::GetInstance().GetTrainingSample();

  // Assert
  EXPECT_EQ(32U, training_sample.size());
}

TEST_F(BraveAdsPredictorsManagerTest, GetTrainingSampleWithSetters) {
  // Arrange
  SetNotificationAdServedAtPredictorVariable(Now());

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kClicked);

  // Act
  const std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample =
      PredictorsManager::GetInstance().GetTrainingSample();

  // Assert
  EXPECT_EQ(34U, training_sample.size());
}

}  // namespace brave_ads
