/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_

#include <map>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/ml/data/vector_data.h"

namespace ads {
namespace ml {
namespace pipeline {

struct EmbeddingPipelineInfo final {
  EmbeddingPipelineInfo();
  EmbeddingPipelineInfo(EmbeddingPipelineInfo&& info) noexcept;
  EmbeddingPipelineInfo& operator=(EmbeddingPipelineInfo&& info) noexcept;
  ~EmbeddingPipelineInfo();

  bool FromValue(const base::Value::Dict& root);

  int version = 0;
  base::Time time;
  std::string locale;
  int dim = 0;
  std::map<std::string, VectorData> embeddings;
};

}  // namespace pipeline
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_
