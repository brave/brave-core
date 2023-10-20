/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_

#include <memory>
#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"
#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

namespace brave_ads::ml::pipeline {

struct PipelineInfo;

class TextProcessing final {
 public:
  static base::expected<TextProcessing, std::string> CreateFromFlatBuffers(
      std::string buffer);
  static base::expected<TextProcessing, std::string> CreateFromValue(
      base::Value::Dict dict);
  static PredictionMap FilterPredictions(const PredictionMap& predictions);

  TextProcessing();
  TextProcessing(TransformationVector transformations,
                 absl::optional<LinearModel> linear_model);

  TextProcessing(const TextProcessing&) = delete;
  TextProcessing& operator=(const TextProcessing&) = delete;

  TextProcessing(TextProcessing&&) noexcept;
  TextProcessing& operator=(TextProcessing&&) noexcept;

  ~TextProcessing();

  bool IsInitialized() const { return is_initialized_; }

  void SetPipeline(PipelineInfo pipeline);
  bool SetPipeline(base::Value::Dict dict);
  bool SetPipeline(std::string buffer);

  absl::optional<PredictionMap> Predict(VectorData* vector_data) const;

  absl::optional<PredictionMap> Apply(
      std::unique_ptr<Data> mutable_input_data) const;

  absl::optional<PredictionMap> GetPredictions(const std::string& text) const;

  absl::optional<PredictionMap> GetTopPredictions(
      const std::string& text) const;

  absl::optional<PredictionMap> ClassifyPage(const std::string& text) const;

 private:
  bool is_initialized_ = false;

  std::string locale_ = "en";

  absl::optional<std::string> pipeline_buffer_;
  TransformationVector transformations_;
  absl::optional<LinearModel> linear_model_;
  absl::optional<NeuralModel> neural_model_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
