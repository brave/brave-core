/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdServedAtPredictorVariableTest
    : public UnitTestBase {};

TEST_F(BraveAdsNotificationAdServedAtPredictorVariableTest, GetDataType) {
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NotificationAdServedAtPredictorVariable>(Now());

  // Act & Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kDouble,
            predictor_variable->GetDataType());
}

TEST_F(BraveAdsNotificationAdServedAtPredictorVariableTest, GetValue) {
  // Arrange
  const base::Time now = TimeFromString("August 19 2019", /*is_local=*/false);
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NotificationAdServedAtPredictorVariable>(now);

  // Act & Assert
  EXPECT_EQ("13210646400000000", predictor_variable->GetValue());
}

TEST_F(BraveAdsNotificationAdServedAtPredictorVariableTest,
       GetValueWithoutTime) {
  // Arrange
  std::unique_ptr<PredictorVariableInterface> predictor_variable =
      std::make_unique<NotificationAdServedAtPredictorVariable>(base::Time());

  // Act & Assert
  EXPECT_EQ("-1", predictor_variable->GetValue());
}

}  // namespace brave_ads
