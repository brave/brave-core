/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/time_since_last_user_activity_event_predictor_variable.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimeSinceLastUserActivityEventPredictorVariableTest
    : public UnitTestBase {};

TEST_F(BraveAdsTimeSinceLastUserActivityEventPredictorVariableTest,
       GetDataType) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<TimeSinceLastUserActivityEventPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::
              kTimeSinceLastOpenedNewTabEvent);

  // Act & Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kInt,
            predictor_variable->GetDataType());
}

TEST_F(BraveAdsTimeSinceLastUserActivityEventPredictorVariableTest,
       GetValueForNoHistory) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<TimeSinceLastUserActivityEventPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::
              kTimeSinceLastOpenedNewTabEvent);

  // Act & Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BraveAdsTimeSinceLastUserActivityEventPredictorVariableTest, GetValue) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<TimeSinceLastUserActivityEventPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::
              kTimeSinceLastOpenedNewTabEvent);

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  AdvanceClockBy(base::Minutes(2));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);

  // Act & Assert
  EXPECT_EQ("120", predictor_variable->GetValue());
}

}  // namespace brave_ads
