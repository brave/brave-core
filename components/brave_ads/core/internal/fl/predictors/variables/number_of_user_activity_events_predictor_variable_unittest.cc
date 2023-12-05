/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/number_of_user_activity_events_predictor_variable.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNumberOfUserActivityEventsPredictorVariableTest
    : public UnitTestBase {};

TEST_F(BraveAdsNumberOfUserActivityEventsPredictorVariableTest, GetDataType) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NumberOfUserActivityEventsPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  // Act & Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kInt,
            predictor_variable->GetDataType());
}

TEST_F(BraveAdsNumberOfUserActivityEventsPredictorVariableTest,
       GetValueWithoutUserActivity) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NumberOfUserActivityEventsPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  // Act & Assert
  EXPECT_EQ("0", predictor_variable->GetValue());
}

TEST_F(BraveAdsNumberOfUserActivityEventsPredictorVariableTest, GetValue) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NumberOfUserActivityEventsPredictorVariable>(
          UserActivityEventType::kOpenedNewTab,
          brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents);

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kBrowserDidResignActive);

  AdvanceClockBy(base::Minutes(31));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kBrowserDidBecomeActive);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act & Assert
  EXPECT_EQ("2", predictor_variable->GetValue());
}

}  // namespace brave_ads
