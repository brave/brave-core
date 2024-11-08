/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/data.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_test_util.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {

constexpr char kOnlyRequiredFieldsNeuralModelPipeline[] =
    "ml/pipeline/text_processing/neural/only_required_fields_neural_model.fb";

constexpr char kValidSegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/linear/valid_segment_classification_min.fb";

constexpr char kEmptySegmentClassificationPipeline[] =
    "ml/pipeline/text_processing/linear/empty_segment_classification.fb";

constexpr char kWrongTransformationOrderPipeline[] =
    "ml/pipeline/text_processing/linear/wrong_transformations_order.fb";

constexpr char kEmptyTransformationsPipeline[] =
    "ml/pipeline/text_processing/linear/empty_transformations_pipeline.fb";

constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/linear/valid_spam_classification.fb";

constexpr char kMissingRequiredFieldClassificationPipeline[] =
    "ml/pipeline/text_processing/linear/"
    "missing_required_field_classification.fb";

constexpr char kInvalidModel[] = "ml/pipeline/text_processing/invalid_model";

constexpr char kNotExistingFile[] =
    "ml/pipeline/text_processing/not_existing_file";

constexpr char kTextCMCCrash[] =
    "ml/pipeline/text_processing/linear/text_cmc_crash.txt";

}  // namespace

class BraveAdsTextProcessingTest : public test::TestBase {};

TEST_F(BraveAdsTextProcessingTest, BuildSimplePipeline) {
  // Arrange
  constexpr double kTolerance = 1e-6;
  constexpr char kTestString[] = "Test String";

  TransformationVector transformations;
  transformations.push_back(std::make_unique<LowercaseTransformation>());
  transformations.push_back(std::make_unique<HashedNGramsTransformation>(
      3, std::vector<uint32_t>{1, 2, 3}));

  const std::vector<float> data_1 = {1.0, 2.0, 3.0};
  const std::vector<float> data_2 = {3.0, 2.0, 1.0};
  const std::vector<float> data_3 = {2.0, 2.0, 2.0};
  const std::map<std::string, VectorData> weights = {
      {"class_1", VectorData(data_1)},
      {"class_2", VectorData(data_2)},
      {"class_3", VectorData(data_3)}};

  const std::map<std::string, float> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  std::string buffer = pipeline::LinearPipelineBufferBuilder()
                           .CreateClassifier(weights, biases)
                           .AddLowercaseTransformation()
                           .Build("en");

  flatbuffers::Verifier verifier(
      reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
  ASSERT_TRUE(linear_text_classification::flat::VerifyModelBuffer(verifier));

  const auto* const raw_model =
      linear_text_classification::flat::GetModel(buffer.data());
  ASSERT_TRUE(raw_model);
  LinearModel linear_model(*raw_model);

  const std::vector<float> data_4 = {1.0, 0.0, 0.0};
  const VectorData data_point_3(data_4);
  const std::optional<PredictionMap> data_point_3_predictions =
      linear_model.Predict(data_point_3);
  ASSERT_TRUE(data_point_3_predictions);
  EXPECT_THAT(*data_point_3_predictions, ::testing::SizeIs(3));

  const pipeline::TextProcessing pipeline = pipeline::TextProcessing(
      std::move(transformations), std::move(linear_model));

  // Act
  const std::optional<PredictionMap> predictions =
      pipeline.GetTopPredictions(kTestString);
  ASSERT_TRUE(predictions);
  ASSERT_GT(predictions->size(), 0U);
  ASSERT_LE(predictions->size(), 3U);

  // Assert
  for (const auto& prediction : *predictions) {
    EXPECT_GT(prediction.second, -kTolerance);
    EXPECT_LT(prediction.second, 1.0 + kTolerance);
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

  const base::FilePath path =
      test::DataPath().AppendASCII(kValidSpamClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  // Act
  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  std::vector<PredictionMap> prediction_maps(train_texts.size());
  for (size_t i = 0; i < train_texts.size(); ++i) {
    std::unique_ptr<Data> text_data =
        std::make_unique<TextData>(train_texts[i]);
    const std::optional<PredictionMap> prediction_map =
        text_processing_pipeline.Apply(std::move(text_data));
    ASSERT_TRUE(prediction_map);
    prediction_maps[i] = *prediction_map;
  }

  // Assert
  for (size_t i = 0; i < prediction_maps.size(); ++i) {
    const PredictionMap& prediction_map = prediction_maps[i];
    for (const auto& pred : prediction_map) {
      const double other_prediction = pred.second;
      EXPECT_GE(prediction_map.at(train_labels[i]), other_prediction);
    }
  }
}

TEST_F(BraveAdsTextProcessingTest, InitValidLinearModelTest) {
  // Arrange
  const base::FilePath path =
      test::DataPath().AppendASCII(kValidSegmentClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, InitValidNeuralModelTest) {
  // Arrange
  const base::FilePath path =
      test::DataPath().AppendASCII(kOnlyRequiredFieldsNeuralModelPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, EmptySegmentModelTest) {
  // Arrange
  const std::string input_text = "This is a spam email.";

  const base::FilePath path =
      test::DataPath().AppendASCII(kEmptySegmentClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  // Act
  std::unique_ptr<Data> text_data = std::make_unique<TextData>(input_text);
  const std::optional<PredictionMap> prediction_map =
      text_processing_pipeline.Apply(std::move(text_data));

  // Assert
  EXPECT_FALSE(prediction_map);
}

TEST_F(BraveAdsTextProcessingTest, EmptyTransformationsModelTest) {
  // Arrange
  const std::string input_text = "This is a spam email.";

  const base::FilePath path =
      test::DataPath().AppendASCII(kEmptyTransformationsPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  // Act
  std::unique_ptr<Data> text_data = std::make_unique<TextData>(input_text);
  const std::optional<PredictionMap> prediction_map =
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

  const base::FilePath path =
      test::DataPath().AppendASCII(kWrongTransformationOrderPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;
  EXPECT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  // Act & Assert
  for (const auto& text : input_texts) {
    std::unique_ptr<Data> text_data = std::make_unique<TextData>(text);
    const std::optional<PredictionMap> prediction_map =
        text_processing_pipeline.Apply(std::move(text_data));
    EXPECT_FALSE(prediction_map);
  }
}

TEST_F(BraveAdsTextProcessingTest, MissingRequiredFieldModelTest) {
  // Arrange
  const base::FilePath path =
      test::DataPath().AppendASCII(kMissingRequiredFieldClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, InvalidModelTest) {
  // Arrange
  const base::FilePath path = test::DataPath().AppendASCII(kInvalidModel);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, kNotExistingFile) {
  // Arrange
  const base::FilePath path = test::DataPath().AppendASCII(kNotExistingFile);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, NotInitializedFileTest) {
  // Arrange
  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(/*file=*/{}));
}

TEST_F(BraveAdsTextProcessingTest, WrongLanguageModelTest) {
  // Arrange
  brave_l10n::test::ScopedDefaultLocale default_locale("es");

  const base::FilePath path =
      test::DataPath().AppendASCII(kValidSegmentClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;

  // Act & Assert
  EXPECT_FALSE(text_processing_pipeline.SetPipeline(std::move(file)));
}

TEST_F(BraveAdsTextProcessingTest, TopPredUnitTest) {
  // Arrange
  constexpr size_t kMaxPredictionsSize = 100;
  constexpr char kTestPage[] = "ethereum bitcoin bat zcash crypto tokens!";

  const base::FilePath path =
      test::DataPath().AppendASCII(kValidSegmentClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  // Act
  const std::optional<PredictionMap> predictions =
      text_processing_pipeline.ClassifyPage(kTestPage);
  ASSERT_TRUE(predictions);

  // Assert
  EXPECT_GT(predictions->size(), 0U);
  EXPECT_LT(predictions->size(), kMaxPredictionsSize);
  EXPECT_TRUE(predictions->count("crypto-crypto"));
  for (const auto& prediction : *predictions) {
    EXPECT_LE(prediction.second, predictions->at("crypto-crypto"));
  }
}

TEST_F(BraveAdsTextProcessingTest, TextCMCCrashTest) {
  // Arrange
  constexpr size_t kMinPredictionsSize = 2;
  constexpr size_t kMaxPredictionsSize = 100;

  const base::FilePath path =
      test::DataPath().AppendASCII(kValidSegmentClassificationPipeline);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);

  pipeline::TextProcessing text_processing_pipeline;
  ASSERT_TRUE(text_processing_pipeline.SetPipeline(std::move(file)));

  const std::optional<std::string> contents =
      test::MaybeReadFileToString(kTextCMCCrash);
  ASSERT_TRUE(contents);

  // Act
  const std::optional<PredictionMap> predictions =
      text_processing_pipeline.ClassifyPage(*contents);
  ASSERT_TRUE(predictions);

  // Assert
  EXPECT_GT(predictions->size(), kMinPredictionsSize);
  EXPECT_LT(predictions->size(), kMaxPredictionsSize);
  EXPECT_TRUE(predictions->count("crypto-crypto"));
  for (const auto& prediction : *predictions) {
    EXPECT_LE(prediction.second, predictions->at("crypto-crypto"));
  }
}

}  // namespace brave_ads::ml
