/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_

#include <memory>
#include <string>

#include "bat/ads/internal/ml/pipeline/embedding_pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_info.h"

namespace base {
class Value;
}  // namespace base

namespace ads::ml::pipeline {

struct EmbeddingPipelineInfo;

class EmbeddingProcessing final {
 public:
  static std::unique_ptr<EmbeddingProcessing> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  bool IsInitialized() const;

  bool SetEmbeddingPipeline(base::Value resource_value);

  TextEmbeddingInfo EmbedText(const std::string& text) const;

 private:
  bool is_initialized_ = false;

  EmbeddingPipelineInfo embedding_pipeline_;
};

}  // namespace ads::ml::pipeline

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
