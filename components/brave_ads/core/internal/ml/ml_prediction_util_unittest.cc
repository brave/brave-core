/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/ml_prediction_util.h"

#include <cmath>
#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsMLPredictionUtilTest : public UnitTestBase {};

TEST_F(BraveAdsMLPredictionUtilTest, SoftmaxTest) {
  // Arrange
  constexpr double kTolerance = 1e-8;

  const std::map<std::string, double> group_1 = {
      {"c1", -1.0}, {"c2", 2.0}, {"c3", 3.0}};

  // Act
  const PredictionMap predictions = Softmax(group_1);

  // Assert
  double sum = 0.0;
  for (auto const& prediction : predictions) {
    sum += prediction.second;
  }
  ASSERT_GT(predictions.at("c3"), predictions.at("c1"));
  ASSERT_GT(predictions.at("c3"), predictions.at("c2"));
  ASSERT_GT(predictions.at("c2"), predictions.at("c1"));
  ASSERT_GT(predictions.at("c1"), 0.0);
  ASSERT_LT(predictions.at("c3"), 1.0);
  EXPECT_LT(sum - 1.0, kTolerance);
}

TEST_F(BraveAdsMLPredictionUtilTest, ExtendedSoftmaxTest) {
  // Arrange
  constexpr double kTolerance = 1e-8;

  const std::map<std::string, double> group_1 = {
      {"c1", 0.0}, {"c2", 1.0}, {"c3", 2.0}};

  const std::map<std::string, double> group_2 = {
      {"c1", 3.0}, {"c2", 4.0}, {"c3", 5.0}};

  // Act
  const PredictionMap predictions_1 = Softmax(group_1);
  const PredictionMap predictions_2 = Softmax(group_2);

  // Assert
  EXPECT_LT(std::fabs(predictions_1.at("c1") - predictions_2.at("c1")),
            kTolerance);
  EXPECT_TRUE(std::fabs(predictions_1.at("c1") - 0.09003057) < kTolerance);

  EXPECT_LT(std::fabs(predictions_1.at("c2") - predictions_2.at("c2")),
            kTolerance);
  EXPECT_TRUE(std::fabs(predictions_1.at("c2") - 0.24472847) < kTolerance);

  EXPECT_LT(std::fabs(predictions_1.at("c3") - predictions_2.at("c3")),
            kTolerance);
  EXPECT_TRUE(std::fabs(predictions_1.at("c3") - 0.66524095) < kTolerance);
}

}  // namespace brave_ads::ml
