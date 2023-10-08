/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/data.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {

constexpr char kValidSegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_segment_classification_min.json";

constexpr char kEmptySegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/empty_segment_classification.json";

constexpr char kWrongTransformationOrderPipeline[] =
    "ml/pipeline/text_processing/wrong_transformations_order.json";

constexpr char kEmptyTransformationsPipeline[] =
    "ml/pipeline/text_processing/empty_transformations_pipeline.json";

constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";

constexpr char kTextCMCCrash[] =
    "ml/pipeline/text_processing/text_cmc_crash.txt";

}  // namespace

class BraveAdsTextProcessingTest : public UnitTestBase {};

TEST_F(BraveAdsTextProcessingTest, BuildSimplePipeline) {
  // Arrange
  constexpr double kTolerance = 1e-6;
  constexpr char kTestString[] = "Test String";

  TransformationVector transformations;
  transformations.push_back(std::make_unique<LowercaseTransformation>());
  transformations.push_back(std::make_unique<HashedNGramsTransformation>(
      3, std::vector<int>{1, 2, 3}));

  const std::vector<float> data_1 = {1.0, 2.0, 3.0};
  const std::vector<float> data_2 = {3.0, 2.0, 1.0};
  const std::vector<float> data_3 = {2.0, 2.0, 2.0};
  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData(data_1)},
      {"class_2", VectorData(data_2)},
      {"class_3", VectorData(data_3)}};

  const std::map<std::string, double> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  const std::vector<float> data_4 = {1.0, 0.0, 0.0};
  const VectorData data_point_3(data_4);

  LinearModel linear_model(weights, biases);
  const PredictionMap data_point_3_predictions =
      linear_model.Predict(data_point_3);
  ASSERT_EQ(3U, data_point_3_predictions.size());

  const pipeline::TextProcessing pipeline = pipeline::TextProcessing(
      std::move(transformations), std::move(linear_model));

  // Act
  const absl::optional<PredictionMap> predictions =
      pipeline.GetTopPredictions(kTestString);
  ASSERT_TRUE(predictions);
  ASSERT_TRUE(!predictions->empty());
  ASSERT_LE(predictions->size(), 3U);

  // Assert
  for (const auto& prediction : *predictions) {
    EXPECT_TRUE(prediction.second > -kTolerance &&
                prediction.second < 1.0 + kTolerance);
  }
}

TEST_F(BraveAdsTextProcessingTest, TestLoadFromValue) {
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

  base::Value::Dict dict = base::test::ParseJsonDict(*json);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));

  std::vector<PredictionMap> prediction_maps(train_texts.size());
  for (size_t i = 0; i < train_texts.size(); ++i) {
    std::unique_ptr<Data> text_data =
        std::make_unique<TextData>(train_texts[i]);
    const absl::optional<PredictionMap> prediction_map =
        text_processing_pipeline.Apply(std::move(text_data));
    ASSERT_TRUE(prediction_map);
    prediction_maps[i] = *prediction_map;
  }

  // Assert
  for (size_t i = 0; i < prediction_maps.size(); ++i) {
    const PredictionMap& prediction_map = prediction_maps[i];
    for (const auto& pred : prediction_map) {
      const double other_prediction = pred.second;
      EXPECT_TRUE(prediction_map.at(train_labels[i]) >= other_prediction);
    }
  }
}

TEST_F(BraveAdsTextProcessingTest, InitValidModelTest) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));
}

TEST_F(BraveAdsTextProcessingTest, EmptySegmentModelTest) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kEmptySegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(std::move(dict)));
}

TEST_F(BraveAdsTextProcessingTest, EmptyTransformationsModelTest) {
  // Arrange
  const std::string input_text = "This is a spam email.";

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kEmptyTransformationsPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);
  pipeline::TextProcessing text_processing_pipeline;
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));

  // Act
  std::unique_ptr<Data> text_data = std::make_unique<TextData>(input_text);
  const absl::optional<PredictionMap> prediction_map =
      text_processing_pipeline.Apply(std::move(text_data));

  // Assert
  EXPECT_FALSE(prediction_map);
}

TEST_F(BraveAdsTextProcessingTest, WrongTransformationsOrderModelTest) {
  // Arrange
  const std::vector<std::string> input_texts = {
      "This is a spam email.", "Another spam trying to sell you viagra",
      "Message from mom with no real subject",
      "Another messase from mom with no real subject", "Yadayada"};

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kWrongTransformationOrderPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);
  pipeline::TextProcessing text_processing_pipeline;
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));

  // Act & Assert
  for (const auto& text : input_texts) {
    std::unique_ptr<Data> text_data = std::make_unique<TextData>(text);
    const absl::optional<PredictionMap> prediction_map =
        text_processing_pipeline.Apply(std::move(text_data));
    EXPECT_FALSE(prediction_map);
  }
}

TEST_F(BraveAdsTextProcessingTest, EmptyModelTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline({}));
}

TEST_F(BraveAdsTextProcessingTest, TopPredUnitTest) {
  // Arrange
  constexpr size_t kMaxPredictionsSize = 100;
  constexpr char kTestPage[] = "ethereum bitcoin bat zcash crypto tokens!";

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));

  // Act
  const absl::optional<PredictionMap> predictions =
      text_processing_pipeline.ClassifyPage(kTestPage);
  ASSERT_TRUE(predictions);

  // Assert
  EXPECT_FALSE(predictions->empty());
  EXPECT_LT(predictions->size(), kMaxPredictionsSize);
  EXPECT_TRUE(predictions->count("crypto-crypto"));
  for (const auto& prediction : *predictions) {
    EXPECT_TRUE(prediction.second <= predictions->at("crypto-crypto"));
  }
}

TEST_F(BraveAdsTextProcessingTest, TextCMCCrashTest) {
  // Arrange
  constexpr size_t kMinPredictionsSize = 2;
  constexpr size_t kMaxPredictionsSize = 100;

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSegmentClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value::Dict dict = base::test::ParseJsonDict(*json);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(dict)));

  const absl::optional<std::string> text =
      ReadFileFromTestPathToString(kTextCMCCrash);
  ASSERT_TRUE(text);

  // Act
  const absl::optional<PredictionMap> predictions =
      text_processing_pipeline.ClassifyPage(*text);
  ASSERT_TRUE(predictions);

  // Assert
  EXPECT_GT(predictions->size(), kMinPredictionsSize);
  EXPECT_LT(predictions->size(), kMaxPredictionsSize);
  EXPECT_TRUE(predictions->count("crypto-crypto"));
  for (const auto& prediction : *predictions) {
    EXPECT_TRUE(prediction.second <= predictions->at("crypto-crypto"));
  }
}

}  // namespace brave_ads::ml
