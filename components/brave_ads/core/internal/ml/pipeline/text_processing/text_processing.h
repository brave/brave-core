/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"

namespace brave_ads::ml::pipeline {

struct PipelineInfo;

class TextProcessing final {
 public:
  static base::expected<TextProcessing, std::string> CreateFromValue(
      base::Value::Dict dict);

  TextProcessing();
  TextProcessing(TransformationVector transformations,
                 model::Linear linear_model);

  TextProcessing(const TextProcessing&) = delete;
  TextProcessing& operator=(const TextProcessing&) = delete;

  TextProcessing(TextProcessing&&) noexcept;
  TextProcessing& operator=(TextProcessing&&) noexcept;

  ~TextProcessing();

  bool IsInitialized() const { return is_initialized_; }

  void SetPipeline(PipelineInfo info);
  bool SetPipeline(base::Value::Dict dict);

  PredictionMap Apply(const std::unique_ptr<Data>& input_data) const;

  PredictionMap GetTopPredictions(const std::string& content) const;

  PredictionMap ClassifyPage(const std::string& content) const;

 private:
  bool is_initialized_ = false;

  uint16_t version_ = 0;
  std::string timestamp_;
  std::string locale_ = "en";
  TransformationVector transformations_;
  model::Linear linear_model_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
