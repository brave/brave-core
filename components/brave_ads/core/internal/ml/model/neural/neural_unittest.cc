/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsNeuralTest : public UnitTestBase {};

TEST_F(BraveAdsNeuralTest, Prediction) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<VectorData> matrix_1 = {VectorData({1.0, 0.0, -3.5}),
                                      VectorData({0.0, 2.2, 8.3})};
  std::vector<VectorData> matrix_2 = {VectorData({-0.5, 1.6}),
                                      VectorData({4.38, -1.0}),
                                      VectorData({2.0, 1.0})};
  std::vector<std::vector<VectorData>> matricies = {matrix_1, matrix_2};

  std::vector<std::string> post_matrix_functions = {"tanh", "softmax"};

  std::vector<std::string> classes = {"class_1", "class_2", "class_3"};

  const NeuralModel neural(matricies, post_matrix_functions, classes);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const PredictionMap sample_predictions = neural.Predict(sample_observation);

  // Assert
  EXPECT_TRUE(
      (std::fabs(0.78853326 - sample_predictions.at("class_1")) < kTolerance) &&
      (std::fabs(0.01296594 - sample_predictions.at("class_2")) < kTolerance) &&
      (std::fabs(0.19850080 - sample_predictions.at("class_3")) < kTolerance));
}

TEST_F(BraveAdsNeuralTest, TopPredictions) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<VectorData> matrix_1 = {VectorData({1.0, 0.0, -3.5}),
                                      VectorData({0.0, 2.2, 8.3})};
  std::vector<VectorData> matrix_2 = {VectorData({-0.5, 1.6}),
                                      VectorData({4.38, -1.0}),
                                      VectorData({2.0, 1.0})};
  std::vector<std::vector<VectorData>> matricies = {matrix_1, matrix_2};

  std::vector<std::string> post_matrix_functions = {"tanh", "softmax"};

  std::vector<std::string> classes = {"class_1", "class_2", "class_3"};

  const NeuralModel neural(matricies, post_matrix_functions, classes);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const PredictionMap sample_predictions =
      neural.GetTopPredictions(sample_observation);
  const PredictionMap sample_predictions_constrained =
      neural.GetTopCountPredictions(sample_observation, 2);

  // Assert
  EXPECT_TRUE((sample_predictions.size() == 3) &&
              sample_predictions_constrained.size() == 2);
  EXPECT_TRUE(
      (std::fabs(0.78853326 - sample_predictions.at("class_1")) < kTolerance) &&
      (std::fabs(0.01296594 - sample_predictions.at("class_2")) < kTolerance) &&
      (std::fabs(0.19850080 - sample_predictions.at("class_3")) < kTolerance));
  EXPECT_TRUE(
      (std::fabs(0.78853326 - sample_predictions_constrained.at("class_1")) <
       kTolerance) &&
      (std::fabs(0.19850080 - sample_predictions_constrained.at("class_3")) <
       kTolerance));
}

}  // namespace brave_ads::ml
