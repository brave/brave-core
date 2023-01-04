/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_

#include <cstdint>
#include <memory>
#include <string>

#include "bat/ads/internal/ml/ml_alias.h"
#include "bat/ads/internal/ml/model/linear/linear.h"

namespace base {
class Value;
}  // namespace base

namespace ads::ml::pipeline {

struct PipelineInfo;

class TextProcessing final {
 public:
  static std::unique_ptr<TextProcessing> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  TextProcessing();
  TextProcessing(TransformationVector transformations,
                 model::Linear linear_model);

  TextProcessing(const TextProcessing& pipeline) = delete;
  TextProcessing& operator=(const TextProcessing& pipeline) = delete;

  TextProcessing(TextProcessing&& other) noexcept = delete;
  TextProcessing& operator=(TextProcessing&& other) noexcept = delete;

  ~TextProcessing();

  bool IsInitialized() const;

  void SetPipeline(PipelineInfo info);
  bool SetPipeline(base::Value resource_value);

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

}  // namespace ads::ml::pipeline

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_TEXT_PROCESSING_H_
