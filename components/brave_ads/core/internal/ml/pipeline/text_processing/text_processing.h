/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/memory_mapped_file.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"
#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"

namespace base {
class File;
}  // namespace base

namespace brave_ads::ml::pipeline {

struct PipelineInfo;

class TextProcessing final {
 public:
  TextProcessing();
  TextProcessing(TransformationVector transformations,
                 std::optional<LinearModel> linear_model);

  TextProcessing(const TextProcessing&) = delete;
  TextProcessing& operator=(const TextProcessing&) = delete;

  TextProcessing(TextProcessing&&) noexcept;
  TextProcessing& operator=(TextProcessing&&) noexcept;

  ~TextProcessing();

  bool IsInitialized() const { return is_initialized_; }
  bool IsNeuralPipline() const { return neural_model_.has_value(); }

  base::expected<bool, std::string> LoadPipeline(base::File file);
  void SetPipeline(PipelineInfo pipeline);
  bool SetPipeline(base::File file);

  std::optional<PredictionMap> Predict(const VectorData* vector_data) const;

  std::optional<PredictionMap> Apply(
      std::unique_ptr<Data> mutable_input_data) const;

  std::optional<PredictionMap> GetPredictions(const std::string& text) const;

  std::optional<PredictionMap> GetTopPredictions(const std::string& text) const;

  std::optional<PredictionMap> ClassifyPage(const std::string& text) const;

 private:
  bool is_initialized_ = false;

  std::string locale_ = "en";

  std::unique_ptr<base::MemoryMappedFile> pipeline_mapped_file_;
  TransformationVector transformations_;
  std::optional<LinearModel> linear_model_;
  std::optional<NeuralModel> neural_model_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
