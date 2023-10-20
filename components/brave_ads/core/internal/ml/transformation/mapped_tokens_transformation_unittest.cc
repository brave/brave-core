/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

#include <cstddef>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_transformation_generated.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {

std::string BuildRawNeuralModel(
    const int vector_dimension,
    const std::map<std::string, std::vector<uint8_t>>&
        token_categories_mapping) {
  flatbuffers::FlatBufferBuilder builder;

  std::vector<
      ::flatbuffers::Offset<text_classification::flat::StringToNumbersMap>>
      mapping_data;
  for (const auto& [name, numbers] : token_categories_mapping) {
    auto numbers_data = builder.CreateVector(numbers);
    auto map_data = text_classification::flat::CreateStringToNumbersMap(
        builder, builder.CreateString(name), numbers_data);
    mapping_data.push_back(map_data);
  }
  auto mapping = builder.CreateVector(mapping_data);

  auto mapped_token_transformation =
      text_classification::flat::CreateMappedTokenTransformation(
          builder, vector_dimension, mapping,
          builder.CreateString("MAPPED_TOKENS"));
  auto transformation_entry =
      text_classification::flat::CreateTransformationEntry(
          builder,
          text_classification::flat::Transformation_MappedTokenTransformation,
          mapped_token_transformation.Union());
  std::vector<
      flatbuffers::Offset<text_classification::flat::TransformationEntry>>
      transformations_data;
  transformations_data.push_back(transformation_entry);
  auto transformations = builder.CreateVector(transformations_data);

  text_classification::flat::NeuralModelBuilder neural_model_builder(builder);
  neural_model_builder.add_transformations(transformations);
  builder.Finish(neural_model_builder.Finish());

  std::string buffer(reinterpret_cast<char*>(builder.GetBufferPointer()),
                     builder.GetSize());

  return buffer;
}

}  // namespace

class BraveAdsMappedTokensTransformationTest : public UnitTestBase {
 public:
  absl::optional<MappedTokensTransformation> BuildMappedTokensTransformation(
      const int vector_dimension,
      const std::map<std::string, std::vector<uint8_t>>&
          token_categories_mapping) {
    buffer_ = BuildRawNeuralModel(vector_dimension, token_categories_mapping);
    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t*>(buffer_.data()), buffer_.size());
    if (!text_classification::flat::VerifyNeuralModelBuffer(verifier)) {
      return absl::nullopt;
    }

    const auto* raw_model =
        text_classification::flat::GetNeuralModel(buffer_.data());
    if (!raw_model || !raw_model->transformations()) {
      return absl::nullopt;
    }

    const auto* transformation_entry = raw_model->transformations()->Get(0);
    if (!transformation_entry) {
      return absl::nullopt;
    }

    const auto* transformation =
        transformation_entry->transformation_as_MappedTokenTransformation();
    if (!transformation) {
      return absl::nullopt;
    }

    return MappedTokensTransformation(transformation);
  }

 private:
  std::string buffer_;
};

TEST_F(BraveAdsMappedTokensTransformationTest, ToMappedTokens) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  constexpr char kTestString[] = "this is a simple test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  int vector_dimension = 6;
  std::map<std::string, std::vector<uint8_t>> token_categories_mapping = {
      {"is", {1}}, {"this", {5}}, {"test-string", {0, 3}}, {"simple", {1, 4}}};

  absl::optional<MappedTokensTransformation> to_mapped_tokens =
      BuildMappedTokensTransformation(vector_dimension,
                                      token_categories_mapping);
  ASSERT_TRUE(to_mapped_tokens);

  // Act
  data = to_mapped_tokens->Apply(data);
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());

  std::vector<float> transformed_vector_values(
      transformed_vector_data->GetDimensionCount());
  transformed_vector_values =
      transformed_vector_data->GetData(transformed_vector_values);

  // Assert
  ASSERT_EQ(DataType::kVector, data->GetType());
  ASSERT_TRUE(transformed_vector_values.size() ==
              static_cast<size_t>(vector_dimension));
  EXPECT_TRUE((std::fabs(1.0 - transformed_vector_values.at(0)) < kTolerance) &&
              (std::fabs(2.0 - transformed_vector_values.at(1)) < kTolerance) &&
              (std::fabs(0.0 - transformed_vector_values.at(2)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(3)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(4)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(5)) < kTolerance));
}

TEST_F(BraveAdsMappedTokensTransformationTest, EmptyText) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  constexpr char kTestString[] = "";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  int vector_dimension = 6;
  std::map<std::string, std::vector<uint8_t>> token_categories_mapping = {
      {"is", {1}}, {"this", {5}}, {"test-string", {0, 3}}, {"simple", {1, 4}}};

  absl::optional<MappedTokensTransformation> to_mapped_tokens =
      BuildMappedTokensTransformation(vector_dimension,
                                      token_categories_mapping);
  ASSERT_TRUE(to_mapped_tokens);

  // Act
  data = to_mapped_tokens->Apply(data);
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());

  std::vector<float> transformed_vector_values(
      transformed_vector_data->GetDimensionCount());
  transformed_vector_values =
      transformed_vector_data->GetData(transformed_vector_values);

  // Assert
  ASSERT_EQ(DataType::kVector, data->GetType());
  ASSERT_TRUE(transformed_vector_values.size() ==
              static_cast<size_t>(vector_dimension));
  EXPECT_TRUE((std::fabs(0 - transformed_vector_values.at(0)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(1)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(2)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(3)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(4)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(5)) < kTolerance));
}

TEST_F(BraveAdsMappedTokensTransformationTest, EmptyMap) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  constexpr char kTestString[] = "this is a simple test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  int vector_dimension = 6;
  std::map<std::string, std::vector<uint8_t>> token_categories_mapping = {};

  absl::optional<MappedTokensTransformation> to_mapped_tokens =
      BuildMappedTokensTransformation(vector_dimension,
                                      token_categories_mapping);
  ASSERT_TRUE(to_mapped_tokens);

  // Act
  data = to_mapped_tokens->Apply(data);
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());

  std::vector<float> transformed_vector_values(
      transformed_vector_data->GetDimensionCount());
  transformed_vector_values =
      transformed_vector_data->GetData(transformed_vector_values);

  // Assert
  ASSERT_EQ(DataType::kVector, data->GetType());
  ASSERT_TRUE(transformed_vector_values.size() ==
              static_cast<size_t>(vector_dimension));
  EXPECT_TRUE((std::fabs(0 - transformed_vector_values.at(0)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(1)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(2)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(3)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(4)) < kTolerance) &&
              (std::fabs(0 - transformed_vector_values.at(5)) < kTolerance));
}

TEST_F(BraveAdsMappedTokensTransformationTest, NonTextData) {
  // Arrange
  VectorData vector_data({1.0, 2.0, 4.0, 0.03, 0.0});
  std::unique_ptr<Data> data = std::make_unique<VectorData>(vector_data);

  int vector_dimension = 6;
  std::map<std::string, std::vector<uint8_t>> token_categories_mapping = {
      {"is", {1}}, {"this", {5}}, {"test-string", {0, 3}}, {"simple", {1, 4}}};

  absl::optional<MappedTokensTransformation> to_mapped_tokens =
      BuildMappedTokensTransformation(vector_dimension,
                                      token_categories_mapping);
  ASSERT_TRUE(to_mapped_tokens);

  // Act
  data = to_mapped_tokens->Apply(data);

  // Assert
  EXPECT_TRUE(data == nullptr);
}

}  // namespace brave_ads::ml
