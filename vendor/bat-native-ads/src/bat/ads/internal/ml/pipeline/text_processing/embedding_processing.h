/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_

#include <cstdint>
#include <memory>
#include <string>

#include "bat/ads/internal/ml/data/vector_data.h"

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

  void SetInfo(const PipelineEmbeddingInfo& info);

  bool FromValue(base::Value resource_value);

  std::string CleanText(const std::string& text, bool is_html);

  VectorData EmbedText(const std::string& text) const;

 private:
  bool is_initialized_ = false;
  uint16_t version_ = 0;
  std::string timestamp_ = "";
  std::string locale_ = "en";
  int embeddings_dim_ = 0;
  std::map<std::string, VectorData> embeddings_;
};

}  // namespace pipeline
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_TEXT_PROCESSING_EMBEDDING_PROCESSING_H_
