/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_NEURAL_PIPELINE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_NEURAL_PIPELINE_TEST_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml::pipeline {

class NeuralPipelineBufferBuilder final {
 public:
  NeuralPipelineBufferBuilder();

  NeuralPipelineBufferBuilder(const NeuralPipelineBufferBuilder&) = delete;
  NeuralPipelineBufferBuilder& operator=(const NeuralPipelineBufferBuilder&) =
      delete;

  ~NeuralPipelineBufferBuilder();

  NeuralPipelineBufferBuilder& CreateClassifier(
      const std::vector<std::vector<VectorData>>& raw_matrices,
      const std::vector<std::string>& raw_activation_functions,
      const std::vector<std::string>& raw_segments);

  NeuralPipelineBufferBuilder& AddMappedTokensTransformation(
      int vector_dimension,
      const std::map<std::string, std::vector<uint16_t>>&
          token_categories_mapping);

  std::string Build(const std::string& language);

 private:
  flatbuffers::FlatBufferBuilder builder_;
  flatbuffers::Offset<neural_text_classification::flat::Classifier> classifier_;
  std::vector<
      flatbuffers::Offset<neural_text_classification::flat::Transformation>>
      transformations_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_NEURAL_PIPELINE_TEST_UTIL_H_
