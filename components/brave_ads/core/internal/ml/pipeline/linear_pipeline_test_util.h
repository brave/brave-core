/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_TEST_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml::pipeline {

class LinearPipelineBufferBuilder final {
 public:
  LinearPipelineBufferBuilder();

  LinearPipelineBufferBuilder(const LinearPipelineBufferBuilder&) = delete;
  LinearPipelineBufferBuilder& operator=(const LinearPipelineBufferBuilder&) =
      delete;

  ~LinearPipelineBufferBuilder();

  LinearPipelineBufferBuilder& CreateClassifier(
      const std::map<std::string, VectorData>& raw_weights,
      const std::map<std::string, float>& raw_biases);

  LinearPipelineBufferBuilder& AddLowercaseTransformation();

  LinearPipelineBufferBuilder& AddHashedNGramsTransformation(
      int bucket_count,
      const std::vector<uint32_t>& subgrams);

  std::string Build(const std::string& language);

 private:
  flatbuffers::FlatBufferBuilder builder_;
  flatbuffers::Offset<linear_text_classification::flat::Classifier> classifier_;
  std::vector<
      flatbuffers::Offset<linear_text_classification::flat::Transformation>>
      transformations_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_LINEAR_PIPELINE_TEST_UTIL_H_
