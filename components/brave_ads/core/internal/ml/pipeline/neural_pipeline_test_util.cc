/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_test_util.h"

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_transformation_generated.h"

namespace brave_ads::ml::pipeline {

NeuralPipelineBufferBuilder::NeuralPipelineBufferBuilder() = default;

NeuralPipelineBufferBuilder::~NeuralPipelineBufferBuilder() = default;

NeuralPipelineBufferBuilder& NeuralPipelineBufferBuilder::CreateClassifier(
    const std::vector<std::vector<VectorData>>& raw_matrices,
    const std::vector<std::string>& raw_activation_functions,
    const std::vector<std::string>& raw_segments) {
  std::vector<flatbuffers::Offset<flatbuffers::String>>
      activation_functions_data;
  activation_functions_data.reserve(raw_activation_functions.size());
  for (const auto& func : raw_activation_functions) {
    activation_functions_data.push_back(builder_.CreateString(func));
  }
  auto activation_functions = builder_.CreateVector(activation_functions_data);

  std::vector<flatbuffers::Offset<flatbuffers::String>> segments_data;
  segments_data.reserve(raw_segments.size());
  for (const auto& cls : raw_segments) {
    segments_data.push_back(builder_.CreateString(cls));
  }
  auto segments = builder_.CreateVector(segments_data);

  std::vector<flatbuffers::Offset<neural_text_classification::flat::Matrix>>
      matrices_data;
  matrices_data.reserve(raw_matrices.size());
  for (const auto& matrix : raw_matrices) {
    std::vector<
        ::flatbuffers::Offset<neural_text_classification::flat::WeightsRow>>
        weights_rows_data;
    weights_rows_data.reserve(matrix.size());
    for (const auto& row : matrix) {
      auto weights_row = builder_.CreateVector(row.GetData());
      weights_rows_data.push_back(
          neural_text_classification::flat::CreateWeightsRow(builder_,
                                                             weights_row));
    }
    auto weights_rows = builder_.CreateVector(weights_rows_data);
    matrices_data.push_back(
        neural_text_classification::flat::CreateMatrix(builder_, weights_rows));
  }
  auto matrices = builder_.CreateVector(matrices_data);

  classifier_ = neural_text_classification::flat::CreateClassifier(
      builder_, builder_.CreateString("NEURAL"), segments, matrices,
      activation_functions);

  return *this;
}

NeuralPipelineBufferBuilder&
NeuralPipelineBufferBuilder::AddMappedTokensTransformation(
    int vector_dimension,
    const std::map<std::string, std::vector<uint16_t>>&
        token_categories_mapping) {
  std::vector<::flatbuffers::Offset<
      neural_text_classification::flat::TokenToSegmentIndices>>
      mapping_data;
  for (const auto& [token, indices] : token_categories_mapping) {
    auto indices_data = builder_.CreateVector(indices);
    auto map_data =
        neural_text_classification::flat::CreateTokenToSegmentIndices(
            builder_, builder_.CreateString(token), indices_data);
    mapping_data.push_back(map_data);
  }
  auto mapping = builder_.CreateVector(mapping_data);

  auto mapped_token_transformation =
      neural_text_classification::flat::CreateMappedTokenTransformation(
          builder_, vector_dimension, mapping);
  auto transformation_entry =
      neural_text_classification::flat::CreateTransformation(
          builder_,
          neural_text_classification::flat::
              TransformationType_MappedTokenTransformation,
          mapped_token_transformation.Union());
  transformations_.push_back(transformation_entry);

  return *this;
}

std::string NeuralPipelineBufferBuilder::Build(const std::string& language) {
  auto transformations = builder_.CreateVector(transformations_);
  const auto language_offset = builder_.CreateString(language);

  neural_text_classification::flat::ModelBuilder neural_model_builder(builder_);
  neural_model_builder.add_locale(language_offset);
  neural_model_builder.add_classifier(classifier_);
  neural_model_builder.add_transformations(transformations);
  builder_.Finish(neural_model_builder.Finish());

  std::string buffer(reinterpret_cast<char*>(builder_.GetBufferPointer()),
                     builder_.GetSize());
  return buffer;
}

}  // namespace brave_ads::ml::pipeline
