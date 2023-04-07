/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/ml/pipeline/embedding_pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"

namespace base {
class Value;
}  // namespace base

namespace brave_ads::ml::pipeline {

struct EmbeddingPipelineInfo;

class EmbeddingProcessing final {
 public:
  static std::unique_ptr<EmbeddingProcessing> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  bool IsInitialized() const { return is_initialized_; }

  bool SetEmbeddingPipeline(base::Value resource_value);

  TextEmbeddingInfo EmbedText(const std::string& text) const;

 private:
  bool is_initialized_ = false;

  EmbeddingPipelineInfo embedding_pipeline_;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
