/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsNeuralTest : public test::TestBase {
 public:
  std::optional<NeuralModel> BuildNeuralModel(
      const std::vector<std::vector<VectorData>>& raw_matrices,
      const std::vector<std::string>& raw_activation_functions,
      const std::vector<std::string>& raw_segments) {
    buffer_ = pipeline::NeuralPipelineBufferBuilder()
                  .CreateClassifier(raw_matrices, raw_activation_functions,
                                    raw_segments)
                  .AddMappedTokensTransformation(0, {})
                  .Build("en");

    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t*>(buffer_.data()), buffer_.size());
    if (!neural_text_classification::flat::VerifyModelBuffer(verifier)) {
      return std::nullopt;
    }

    const auto* const raw_model =
        neural_text_classification::flat::GetModel(buffer_.data());
    if (!raw_model) {
      return std::nullopt;
    }

    return NeuralModel(*raw_model);
  }

 private:
  std::string buffer_;
};

TEST_F(BraveAdsNeuralTest, Prediction) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<VectorData> matrix_1 = {VectorData({1.0, 0.0, -3.5}),
                                      VectorData({0.0, 2.2, 8.3})};
  std::vector<VectorData> matrix_2 = {VectorData({-0.5, 1.6}),
                                      VectorData({4.38, -1.0}),
                                      VectorData({2.0, 1.0})};
  std::vector<std::vector<VectorData>> matrices = {matrix_1, matrix_2};

  std::vector<std::string> activation_functions = {"tanh", "softmax"};

  std::vector<std::string> segments = {"class_1", "class_2", "class_3"};

  std::optional<NeuralModel> neural(
      BuildNeuralModel(matrices, activation_functions, segments));
  ASSERT_TRUE(neural);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const std::optional<PredictionMap> sample_predictions =
      neural->Predict(sample_observation);
  ASSERT_TRUE(sample_predictions);

  // Assert
  EXPECT_LT(std::fabs(0.78853326 - sample_predictions->at("class_1")),
            kTolerance);
  EXPECT_LT(std::fabs(0.01296594 - sample_predictions->at("class_2")),
            kTolerance);
  EXPECT_LT(std::fabs(0.19850080 - sample_predictions->at("class_3")),
            kTolerance);
}

TEST_F(BraveAdsNeuralTest, PredictionNomatrices) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<std::vector<VectorData>> matrices;
  std::vector<std::string> activation_functions;
  std::vector<std::string> segments = {"class_1", "class_2", "class_3"};

  std::optional<NeuralModel> neural(
      BuildNeuralModel(matrices, activation_functions, segments));
  ASSERT_TRUE(neural);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const std::optional<PredictionMap> sample_predictions =
      neural->Predict(sample_observation);
  ASSERT_TRUE(sample_predictions);

  // Assert
  EXPECT_LT(std::fabs(0.2 - sample_predictions->at("class_1")), kTolerance);
  EXPECT_LT(std::fabs(0.65 - sample_predictions->at("class_2")), kTolerance);
  EXPECT_LT(std::fabs(0.15 - sample_predictions->at("class_3")), kTolerance);
}

TEST_F(BraveAdsNeuralTest, PredictionDefaultPostMatrixFunctions) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<VectorData> matrix_1 = {VectorData({1.0, 0.0, -3.5}),
                                      VectorData({0.0, 2.2, 8.3})};
  std::vector<VectorData> matrix_2 = {VectorData({-0.5, 1.6}),
                                      VectorData({4.38, -1.0}),
                                      VectorData({2.0, 1.0})};
  std::vector<std::vector<VectorData>> matrices = {matrix_1, matrix_2};

  std::vector<std::string> activation_functions = {"tanh_misspelled", "none"};

  std::vector<std::string> segments = {"class_1", "class_2", "class_3"};

  std::optional<NeuralModel> neural(
      BuildNeuralModel(matrices, activation_functions, segments));
  ASSERT_TRUE(neural);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const std::optional<PredictionMap> sample_predictions =
      neural->Predict(sample_observation);
  ASSERT_TRUE(sample_predictions);

  // Assert
  EXPECT_LT(std::fabs(4.4425 - sample_predictions->at("class_1")), kTolerance);
  EXPECT_LT(std::fabs(-4.0985 - sample_predictions->at("class_2")), kTolerance);
  EXPECT_LT(std::fabs(2.025 - sample_predictions->at("class_3")), kTolerance);
}

TEST_F(BraveAdsNeuralTest, TopPredictions) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  std::vector<VectorData> matrix_1 = {VectorData({1.0, 0.0, -3.5}),
                                      VectorData({0.0, 2.2, 8.3})};
  std::vector<VectorData> matrix_2 = {VectorData({-0.5, 1.6}),
                                      VectorData({4.38, -1.0}),
                                      VectorData({2.0, 1.0})};
  std::vector<std::vector<VectorData>> matrices = {matrix_1, matrix_2};

  std::vector<std::string> activation_functions = {"tanh", "softmax"};

  std::vector<std::string> segments = {"class_1", "class_2", "class_3"};

  std::optional<NeuralModel> neural(
      BuildNeuralModel(matrices, activation_functions, segments));
  ASSERT_TRUE(neural);
  const VectorData sample_observation({0.2, 0.65, 0.15});

  // Act
  const std::optional<PredictionMap> sample_predictions =
      neural->GetTopPredictions(sample_observation);
  ASSERT_TRUE(sample_predictions);
  ASSERT_THAT(*sample_predictions, ::testing::SizeIs(3));

  const std::optional<PredictionMap> sample_predictions_constrained =
      neural->GetTopCountPredictions(sample_observation, 2);
  ASSERT_TRUE(sample_predictions_constrained);
  ASSERT_THAT(*sample_predictions_constrained, ::testing::SizeIs(2));

  // Assert
  EXPECT_LT(std::fabs(0.78853326 - sample_predictions->at("class_1")),
            kTolerance);
  EXPECT_LT(std::fabs(0.01296594 - sample_predictions->at("class_2")),
            kTolerance);
  EXPECT_LT(std::fabs(0.19850080 - sample_predictions->at("class_3")),
            kTolerance);
  EXPECT_LT(
      std::fabs(0.78853326 - sample_predictions_constrained->at("class_1")),
      kTolerance);
  EXPECT_LT(
      std::fabs(0.19850080 - sample_predictions_constrained->at("class_3")),
      kTolerance);
}

}  // namespace brave_ads::ml
