/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/ad_last_seen_hours_ago_predictor_variable.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/ad_last_seen_hours_ago_predictor_variable_util.h"

#include <sstream>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdLastSeenHoursAgoTest : public UnitTestBase {};

TEST_F(BraveAdsAdLastSeenHoursAgoTest, GetDataType) {
  // Arrange
  const AdLastSeenHoursAgoPredictorVariable predictor_variable(10.0);

  // Act

  // Assert
  EXPECT_EQ(brave_federated::mojom::DataType::kString,
            predictor_variable.GetDataType());
}

TEST_F(BraveAdsAdLastSeenHoursAgoTest, GetValue) {
  // Arrange
  const AdLastSeenHoursAgoPredictorVariable predictor_variable(0.0);
  // std::unique_ptr<PredictorVariableInterface> predictor_variable =
  //     std::make_unique<AdLastSeenHoursAgoPredictorVariable>(0.0);
  SetAdLastSeenHoursAgoPredictorVariable(10.0);

  // Act
  const std::string value = predictor_variable.GetValue();

  // Assert
  EXPECT_EQ('10.0', value);
}

} // namespace brave_ads
