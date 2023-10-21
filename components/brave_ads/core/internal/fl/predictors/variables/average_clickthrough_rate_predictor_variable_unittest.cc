/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/average_clickthrough_rate_predictor_variable.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAverageClickthroughRatePredictorVariableTest
    : public UnitTestBase {};

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest, GetDataType) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(7));

  // Act & Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble,
            predictor_variable->GetDataType());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForNoHistory) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  // Act & Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest,
       GetValueAfterExceedingTimeWindow) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  AdvanceClockBy(base::Days(2));

  // Act & Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForNoClicks) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act & Assert
  EXPECT_EQ("0", predictor_variable->GetValue());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForOneClick) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_EQ("1", predictor_variable->GetValue());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest,
       GetValueForMoreThanOneClick) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

TEST_F(BraveAdsAverageClickthroughRatePredictorVariableTest, GetValue) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<AverageClickthroughRatePredictorVariable>(
          /*time_window=*/base::Days(1));

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  // Act & Assert
  EXPECT_EQ("0.3333333333333333", predictor_variable->GetValue());
}

}  // namespace brave_ads
