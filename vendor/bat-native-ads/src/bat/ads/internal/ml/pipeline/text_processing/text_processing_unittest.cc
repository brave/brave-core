/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <fstream>
#include <map>
#include <vector>

#include "bat/ads/internal/ml/data/data.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/model/linear/linear.h"
#include "bat/ads/internal/ml/pipeline/pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"
#include "bat/ads/internal/ml/transformation/transformation.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

namespace {

const char kValidSegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_segment_classification_min.json";

const char kInvalidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/invalid_spam_classification.json";

const char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";

const char kTextCMCCrash[] = "ml/pipeline/text_processing/text_cmc_crash.txt";

}  // namespace

class BatAdsTextProcessingPipelineTest : public UnitTestBase {
 protected:
  BatAdsTextProcessingPipelineTest() = default;

  ~BatAdsTextProcessingPipelineTest() override = default;
};

TEST_F(BatAdsTextProcessingPipelineTest, BuildSimplePipeline) {
  // Arrange
  const double kTolerance = 1e-6;
  const unsigned kExpectedLen = 3;
  const std::string kTestString = "Test String";

  TransformationVector transformations;
  LowercaseTransformation lowercase;
  transformations.push_back(
      std::make_unique<LowercaseTransformation>(lowercase));
  HashedNGramsTransformation hashed_ngrams(3, std::vector<int>{1, 2, 3});
  transformations.push_back(
      std::make_unique<HashedNGramsTransformation>(hashed_ngrams));

  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData(std::vector<double>{1.0, 2.0, 3.0})},
      {"class_2", VectorData(std::vector<double>{3.0, 2.0, 1.0})},
      {"class_3", VectorData(std::vector<double>{2.0, 2.0, 2.0})}};

  const std::map<std::string, double> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  const model::Linear linear_model(weights, biases);
  const pipeline::TextProcessing pipeline =
      pipeline::TextProcessing(transformations, linear_model);

  const VectorData data_point_3(std::vector<double>{1.0, 0.0, 0.0});

  // Act
  const PredictionMap data_point_3_predictions =
      linear_model.Predict(data_point_3);
  const PredictionMap predictions = pipeline.GetTopPredictions(kTestString);

  // Assert
  ASSERT_EQ(kExpectedLen, data_point_3_predictions.size());
  ASSERT_TRUE(predictions.size() && predictions.size() <= kExpectedLen);
  for (const auto& prediction : predictions) {
    EXPECT_TRUE(prediction.second > -kTolerance &&
                prediction.second < 1.0 + kTolerance);
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, TestLoadFromJson) {
  // Arrange
  const std::vector<std::string> train_texts = {
      "This is a spam email.", "Another spam trying to sell you viagra",
      "Message from mom with no real subject",
      "Another messase from mom with no real subject", "Yadayada"};
  const std::vector<std::string> train_labels = {"spam", "spam", "ham", "ham",
                                                 "junk"};

  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kValidSpamClassificationPipeline);
  pipeline::TextProcessing text_processing_pipeline;

  // Act
  ASSERT_TRUE(json_optional.has_value());
  const std::string json = json_optional.value();
  bool load_success = text_processing_pipeline.FromJson(json);
  ASSERT_TRUE(load_success);

  std::vector<PredictionMap> prediction_maps(train_texts.size());
  for (size_t i = 0; i < train_texts.size(); i++) {
    const std::unique_ptr<Data> text_data =
        std::make_unique<TextData>(TextData(train_texts[i]));
    const PredictionMap prediction_map =
        text_processing_pipeline.Apply(text_data);
    prediction_maps[i] = prediction_map;
  }

  // Assert
  for (size_t i = 0; i < prediction_maps.size(); i++) {
    const PredictionMap& prediction_map = prediction_maps[i];
    for (const auto& pred : prediction_map) {
      const double other_prediction = pred.second;
      EXPECT_TRUE(prediction_map.at(train_labels[i]) >= other_prediction);
    }
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, InitValidModelTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;
  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);

  // Act
  ASSERT_TRUE(json_optional.has_value());
  const std::string json = json_optional.value();
  bool loaded_successfully = text_processing_pipeline.FromJson(json);

  // Assert
  EXPECT_TRUE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, InvalidModelTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;
  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kInvalidSpamClassificationPipeline);

  // Act
  ASSERT_TRUE(json_optional.has_value());
  const std::string json = json_optional.value();
  bool loaded_successfully = text_processing_pipeline.FromJson(json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, EmptyModelTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;
  const std::string empty_model_json = "{}";

  // Act
  bool loaded_successfully =
      text_processing_pipeline.FromJson(empty_model_json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, MissingModelTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;

  // Act
  const std::string missing_model_json = "";
  bool loaded_successfully =
      text_processing_pipeline.FromJson(missing_model_json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, TopPredUnitTest) {
  // Arrange
  const size_t kMaxPredictionsSize = 100;
  const std::string kTestPage = "ethereum bitcoin bat zcash crypto tokens!";
  pipeline::TextProcessing text_processing_pipeline;
  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);

  // Act
  ASSERT_TRUE(json_optional.has_value());
  const std::string json = json_optional.value();
  ASSERT_TRUE(text_processing_pipeline.FromJson(json));
  const PredictionMap predictions =
      text_processing_pipeline.ClassifyPage(kTestPage);

  // Assert
  ASSERT_TRUE(predictions.size());
  ASSERT_LT(predictions.size(), kMaxPredictionsSize);
  ASSERT_TRUE(predictions.count("crypto-crypto"));
  for (const auto& prediction : predictions) {
    EXPECT_TRUE(prediction.second <= predictions.at("crypto-crypto"));
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, TextCMCCrashTest) {
  // Arrange
  const size_t kMinPredictionsSize = 2;
  const size_t kMaxPredictionsSize = 100;
  pipeline::TextProcessing text_processing_pipeline;

  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json_optional.has_value());

  const std::string json = json_optional.value();
  ASSERT_TRUE(text_processing_pipeline.FromJson(json));

  const absl::optional<std::string> text_optional =
      ReadFileFromTestPathToString(kTextCMCCrash);

  // Act
  ASSERT_TRUE(text_optional.has_value());
  const std::string text = text_optional.value();
  const PredictionMap predictions = text_processing_pipeline.ClassifyPage(text);

  // Assert
  ASSERT_GT(predictions.size(), kMinPredictionsSize);
  ASSERT_LT(predictions.size(), kMaxPredictionsSize);
  ASSERT_TRUE(predictions.count("personal finance-personal finance"));
  for (const auto& prediction : predictions) {
    EXPECT_TRUE(prediction.second <=
                predictions.at("personal finance-personal finance"));
  }
}

}  // namespace ml
}  // namespace ads
