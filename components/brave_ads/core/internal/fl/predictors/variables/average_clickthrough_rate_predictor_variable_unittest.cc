/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_clickthrough_rate_predictor_variable.h"

#include <memory>

#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsAverageClickthroughRatePredictorVariableTest : public UnitTestBase {
};

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest, GetDataType) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(7));

  // Act

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble,
            predictor_variable->GetDataType());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForNoHistory) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  // Act

  // Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest,
       GetValueAfterExceedingTimeWindow) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  AdvanceClockBy(base::Days(2));

  // Act

  // Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForNoClicks) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act

  // Assert
  EXPECT_EQ("0", predictor_variable->GetValue());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForOneClick) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act

  // Assert
  EXPECT_EQ("1", predictor_variable->GetValue());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForMoreThanOneClick) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act

  // Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BatAdsAverageClickthroughRatePredictorVariableTest, GetValue) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(base::Days(1));

  const NotificationAdInfo ad;
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  // Act

  // Assert
  EXPECT_EQ("0.3333333333333333", predictor_variable->GetValue());
}

}  // namespace brave_ads
