/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_

#include <memory>
#include <string>

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"

namespace base {
class Value;
}

namespace ads {
namespace ml {
namespace pipeline {

struct PipelineEmbeddingInfo;

class EmbeddingProcessing final {
 public:
  static std::unique_ptr<EmbeddingProcessing> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  EmbeddingProcessing();
  ~EmbeddingProcessing();
  EmbeddingProcessing(const EmbeddingProcessing& pipeline) = delete;
  EmbeddingProcessing& operator=(const EmbeddingProcessing& pipeline) = delete;

  bool IsInitialized() const;

  void SetIsInitialized(bool is_initialized);

  void SetEmbeddingPipeline(const PipelineEmbeddingInfo& info);

  bool FromValue(base::Value resource_value);

  std::string SanitizeText(const std::string& text, bool is_html);

  TextEmbeddingData EmbedText(const std::string& text) const;

 private:
  bool is_initialized_ = false;
  struct PipelineEmbeddingInfo embedding_pipeline_;
};

}  // namespace pipeline
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
