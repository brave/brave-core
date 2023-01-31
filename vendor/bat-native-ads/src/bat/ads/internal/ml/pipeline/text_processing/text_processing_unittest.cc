/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"

#include <map>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"
#include "bat/ads/internal/ml/data/data.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/model/linear/linear.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

namespace {

constexpr char kValidSegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_segment_classification_min.json";

constexpr char kEmptySegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/empty_segment_classification.json";

constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";

constexpr char kTextCMCCrash[] =
    "ml/pipeline/text_processing/text_cmc_crash.txt";

}  // namespace

class BatAdsTextProcessingTest : public UnitTestBase {};

TEST_F(BatAdsTextProcessingTest, BuildSimplePipeline) {
  // Arrange
  constexpr double kTolerance = 1e-6;
  constexpr unsigned kExpectedLen = 3;
  constexpr char kTestString[] = "Test String";

  TransformationVector transformations;
  transformations.push_back(std::make_unique<LowercaseTransformation>());
  transformations.push_back(std::make_unique<HashedNGramsTransformation>(
      3, std::vector<int>{1, 2, 3}));

  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData({1.0, 2.0, 3.0})},
      {"class_2", VectorData({3.0, 2.0, 1.0})},
      {"class_3", VectorData({2.0, 2.0, 2.0})}};

  const std::map<std::string, double> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  const VectorData data_point_3({1.0, 0.0, 0.0});

  model::Linear linear_model(weights, biases);
  const PredictionMap data_point_3_predictions =
      linear_model.Predict(data_point_3);

  const pipeline::TextProcessing pipeline = pipeline::TextProcessing(
      std::move(transformations), std::move(linear_model));

  // Act
  const PredictionMap predictions = pipeline.GetTopPredictions(kTestString);

  // Assert
  ASSERT_EQ(kExpectedLen, data_point_3_predictions.size());
  ASSERT_TRUE(!predictions.empty() && predictions.size() <= kExpectedLen);
  for (const auto& prediction : predictions) {
    EXPECT_TRUE(prediction.second > -kTolerance &&
                prediction.second < 1.0 + kTolerance);
  }
}

TEST_F(BatAdsTextProcessingTest, TestLoadFromValue) {
  // Arrange
  const std::vector<std::string> train_texts = {
      "This is a spam email.", "Another spam trying to sell you viagra",
      "Message from mom with no real subject",
      "Another messase from mom with no real subject", "Yadayada"};
  const std::vector<std::string> train_labels = {"spam", "spam", "ham", "ham",
                                                 "junk"};

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSpamClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(value)));

  std::vector<PredictionMap> prediction_maps(train_texts.size());
  for (size_t i = 0; i < train_texts.size(); i++) {
    const std::unique_ptr<Data> text_data =
        std::make_unique<TextData>(train_texts[i]);
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

TEST_F(BatAdsTextProcessingTest, InitValidModelTest) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  const bool success = text_processing_pipeline.SetPipeline(std::move(value));

  // Assert
  EXPECT_TRUE(success);
}

TEST_F(BatAdsTextProcessingTest, EmptySegmentModelTest) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kEmptySegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  const bool success = text_processing_pipeline.SetPipeline(std::move(value));

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsTextProcessingTest, EmptyModelTest) {
  // Arrange
  const std::string json = "{}";

  base::Value value = base::test::ParseJson(json);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  const bool success = text_processing_pipeline.SetPipeline(std::move(value));

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsTextProcessingTest, MissingModelTest) {
  // Arrange

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  const bool success = text_processing_pipeline.SetPipeline(base::Value());

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsTextProcessingTest, TopPredUnitTest) {
  // Arrange
  constexpr size_t kMaxPredictionsSize = 100;
  constexpr char kTestPage[] = "ethereum bitcoin bat zcash crypto tokens!";

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(value)));

  // Act
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

TEST_F(BatAdsTextProcessingTest, TextCMCCrashTest) {
  // Arrange
  constexpr size_t kMinPredictionsSize = 2;
  constexpr size_t kMaxPredictionsSize = 100;

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(value)));

  const absl::optional<std::string> text =
      ReadFileFromTestPathToString(kTextCMCCrash);
  ASSERT_TRUE(text);

  // Act
  const PredictionMap predictions =
      text_processing_pipeline.ClassifyPage(*text);

  // Assert
  ASSERT_GT(predictions.size(), kMinPredictionsSize);
  ASSERT_LT(predictions.size(), kMaxPredictionsSize);
  ASSERT_TRUE(predictions.count("crypto-crypto"));
  for (const auto& prediction : predictions) {
    EXPECT_TRUE(prediction.second <= predictions.at("crypto-crypto"));
  }
}

}  // namespace ads::ml
