/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsLinearTest : public test::TestBase {
 public:
  std::optional<LinearModel> BuildLinearModel(
      const std::map<std::string, VectorData>& raw_weights,
      const std::map<std::string, float>& biases) {
    buffer_ = pipeline::LinearPipelineBufferBuilder()
                  .CreateClassifier(raw_weights, biases)
                  .AddLowercaseTransformation()
                  .Build("en");

    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t*>(buffer_.data()), buffer_.size());
    if (!linear_text_classification::flat::VerifyModelBuffer(verifier)) {
      return std::nullopt;
    }

    const auto* const raw_model =
        linear_text_classification::flat::GetModel(buffer_.data());
    if (!raw_model) {
      return std::nullopt;
    }

    return LinearModel(*raw_model);
  }

 private:
  std::string buffer_;
};

TEST_F(BraveAdsLinearTest, ThreeClassesPredictionTest) {
  // Arrange
  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData({1.0, 0.0, 0.0})},
      {"class_2", VectorData({0.0, 1.0, 0.0})},
      {"class_3", VectorData({0.0, 0.0, 1.0})}};

  const std::map<std::string, float> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  std::optional<LinearModel> linear(BuildLinearModel(weights, biases));
  ASSERT_TRUE(linear);
  const VectorData class_1_vector_data({1.0, 0.0, 0.0});
  const VectorData class_2_vector_data({0.0, 1.0, 0.0});
  const VectorData class_3_vector_data({0.0, 1.0, 2.0});

  // Act
  const std::optional<PredictionMap> predictions_1 =
      linear->Predict(class_1_vector_data);
  ASSERT_TRUE(predictions_1);
  const std::optional<PredictionMap> predictions_2 =
      linear->Predict(class_2_vector_data);
  ASSERT_TRUE(predictions_2);
  const std::optional<PredictionMap> predictions_3 =
      linear->Predict(class_3_vector_data);
  ASSERT_TRUE(predictions_3);

  // Assert
  EXPECT_GT(predictions_1->at("class_1"), predictions_1->at("class_2"));
  EXPECT_GT(predictions_1->at("class_1"), predictions_1->at("class_3"));
  EXPECT_GT(predictions_3->at("class_3"), predictions_3->at("class_1"));
  EXPECT_GT(predictions_2->at("class_2"), predictions_2->at("class_1"));
  EXPECT_GT(predictions_2->at("class_2"), predictions_2->at("class_3"));
  EXPECT_GT(predictions_3->at("class_3"), predictions_3->at("class_2"));
}

TEST_F(BraveAdsLinearTest, BiasesPredictionTest) {
  // Arrange
  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData({1.0, 0.0, 0.0})},
      {"class_2", VectorData({0.0, 1.0, 0.0})},
      {"class_3", VectorData({0.0, 0.0, 1.0})}};

  const std::map<std::string, float> biases = {
      {"class_1", 0.5}, {"class_2", 0.25}, {"class_3", 1.0}};

  std::optional<LinearModel> linear_biased(BuildLinearModel(weights, biases));
  ASSERT_TRUE(linear_biased);
  const VectorData avg_vector({1.0, 1.0, 1.0});

  // Act
  const std::optional<PredictionMap> predictions =
      linear_biased->Predict(avg_vector);
  ASSERT_TRUE(predictions);

  // Assert
  EXPECT_GT(predictions->at("class_3"), predictions->at("class_1"));
  EXPECT_GT(predictions->at("class_3"), predictions->at("class_2"));
  EXPECT_GT(predictions->at("class_1"), predictions->at("class_2"));
}

TEST_F(BraveAdsLinearTest, BinaryClassifierPredictionTest) {
  // Arrange
  const std::vector<float> data = {0.3, 0.2, 0.25};
  const std::map<std::string, VectorData> weights = {
      {"the_only_class", VectorData(data)}};

  const std::map<std::string, float> biases = {{"the_only_class", -0.45}};

  std::optional<LinearModel> linear(BuildLinearModel(weights, biases));
  ASSERT_TRUE(linear);
  const VectorData vector_data_0({1.07, 1.52, 0.91});
  const VectorData vector_data_1({1.11, 1.63, 1.21});

  // Act
  const std::optional<PredictionMap> predictions_0 =
      linear->Predict(vector_data_0);
  ASSERT_TRUE(predictions_0);
  ASSERT_THAT(*predictions_0, ::testing::SizeIs(1));

  const std::optional<PredictionMap> predictions_1 =
      linear->Predict(vector_data_1);
  ASSERT_TRUE(predictions_1);
  ASSERT_THAT(*predictions_1, ::testing::SizeIs(1));

  // Assert
  EXPECT_LE(predictions_0->at("the_only_class"), 0.5);
  EXPECT_GT(predictions_1->at("the_only_class"), 0.5);
}

TEST_F(BraveAdsLinearTest, TopPredictionsTest) {
  // Arrange
  constexpr size_t kPredictionLimits[2] = {2, 1};
  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData({1.0, 0.5, 0.8})},
      {"class_2", VectorData({0.3, 1.0, 0.7})},
      {"class_3", VectorData({0.6, 0.9, 1.0})},
      {"class_4", VectorData({0.7, 1.0, 0.8})},
      {"class_5", VectorData({1.0, 0.2, 1.0})}};

  const std::map<std::string, float> biases = {{"class_1", 0.21},
                                               {"class_2", 0.22},
                                               {"class_3", 0.23},
                                               {"class_4", 0.22},
                                               {"class_5", 0.21}};

  std::optional<LinearModel> linear_biased(BuildLinearModel(weights, biases));
  ASSERT_TRUE(linear_biased);
  const std::vector<float> pt_1 = {1.0, 0.99, 0.98, 0.97, 0.96};
  const std::vector<float> pt_2 = {0.83, 0.79, 0.91, 0.87, 0.82};
  const std::vector<float> pt_3 = {0.92, 0.95, 0.85, 0.91, 0.73};
  const VectorData point_1(pt_1);
  const VectorData point_2(pt_2);
  const VectorData point_3(pt_3);

  // Act
  const std::optional<PredictionMap> predictions_1 =
      linear_biased->GetTopPredictions(point_1);
  ASSERT_TRUE(predictions_1);
  const std::optional<PredictionMap> predictions_2 =
      linear_biased->GetTopCountPredictions(point_2, kPredictionLimits[0]);
  ASSERT_TRUE(predictions_2);
  const std::optional<PredictionMap> predictions_3 =
      linear_biased->GetTopCountPredictions(point_3, kPredictionLimits[1]);
  ASSERT_TRUE(predictions_3);

  // Assert
  EXPECT_EQ(weights.size(), predictions_1->size());
  EXPECT_EQ(kPredictionLimits[0], predictions_2->size());
  EXPECT_EQ(kPredictionLimits[1], predictions_3->size());
}

}  // namespace brave_ads::ml
