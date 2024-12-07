/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_test_util.h"

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_transformation_generated.h"

namespace brave_ads::ml::pipeline {

LinearPipelineBufferBuilder::LinearPipelineBufferBuilder() = default;

LinearPipelineBufferBuilder::~LinearPipelineBufferBuilder() = default;

LinearPipelineBufferBuilder& LinearPipelineBufferBuilder::CreateClassifier(
    const std::map<std::string, VectorData>& raw_weights,
    const std::map<std::string, float>& raw_biases) {
  std::vector<flatbuffers::Offset<
      linear_text_classification::flat::SegmentWeightVector>>
      segment_weight_vectors_data;
  segment_weight_vectors_data.reserve(raw_weights.size());
  for (const auto& [segment, weights_data] : raw_weights) {
    const auto weights = builder_.CreateVector(weights_data.GetData());
    segment_weight_vectors_data.push_back(
        linear_text_classification::flat::CreateSegmentWeightVector(
            builder_, builder_.CreateString(segment), weights));
  }

  const auto segment_weight_vectors =
      builder_.CreateVector(segment_weight_vectors_data);

  std::vector<
      flatbuffers::Offset<linear_text_classification::flat::SegmentBias>>
      biases_data;
  biases_data.reserve(raw_biases.size());
  for (const auto& [segment, bias] : raw_biases) {
    biases_data.push_back(linear_text_classification::flat::CreateSegmentBias(
        builder_, builder_.CreateString(segment), bias));
  }
  const auto biases = builder_.CreateVector(biases_data);

  classifier_ = linear_text_classification::flat::CreateClassifier(
      builder_, builder_.CreateString("LINEAR"), biases,
      segment_weight_vectors);

  return *this;
}

LinearPipelineBufferBuilder&
LinearPipelineBufferBuilder::AddLowercaseTransformation() {
  auto lowercase_transformation =
      linear_text_classification::flat::CreateLowercaseTransformation(builder_);
  auto transformation_entry =
      linear_text_classification::flat::CreateTransformation(
          builder_,
          linear_text_classification::flat::
              TransformationType_LowercaseTransformation,
          lowercase_transformation.Union());
  transformations_.push_back(transformation_entry);

  return *this;
}

LinearPipelineBufferBuilder&
LinearPipelineBufferBuilder::AddHashedNGramsTransformation(
    int bucket_count,
    const std::vector<uint32_t>& subgrams) {
  const auto ngrams_range = builder_.CreateVector(subgrams);
  auto hashed_ngram_transformation =
      linear_text_classification::flat::CreateHashedNGramsTransformation(
          builder_, ngrams_range, bucket_count);
  auto transformation_entry =
      linear_text_classification::flat::CreateTransformation(
          builder_,
          linear_text_classification::flat::
              TransformationType_HashedNGramsTransformation,
          hashed_ngram_transformation.Union());
  transformations_.push_back(transformation_entry);

  return *this;
}

std::string LinearPipelineBufferBuilder::Build(const std::string& language) {
  auto transformations = builder_.CreateVector(transformations_);
  const auto language_offset = builder_.CreateString(language);

  linear_text_classification::flat::ModelBuilder neural_model_builder(builder_);
  neural_model_builder.add_locale(language_offset);
  neural_model_builder.add_classifier(classifier_);
  neural_model_builder.add_transformations(transformations);
  builder_.Finish(neural_model_builder.Finish());

  std::string buffer(reinterpret_cast<char*>(builder_.GetBufferPointer()),
                     builder_.GetSize());
  return buffer;
}

}  // namespace brave_ads::ml::pipeline
