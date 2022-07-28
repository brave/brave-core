/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_

#include <map>
#include <string>

#include "base/time/time.h"
#include "bat/ads/internal/ml/data/vector_data.h"

namespace ads {
namespace ml {
namespace pipeline {

struct PipelineEmbeddingInfo final {
  PipelineEmbeddingInfo();
  PipelineEmbeddingInfo(PipelineEmbeddingInfo&& info) noexcept;
  PipelineEmbeddingInfo& operator=(PipelineEmbeddingInfo&& info) noexcept;
  ~PipelineEmbeddingInfo();

  PipelineEmbeddingInfo(const int version,
                        const base::Time timestamp,
                        const std::string& locale,
                        const int dim,
                        const std::map<std::string, VectorData>& embeddings);

  int version;
  base::Time timestamp;
  std::string locale;
  int dim;
  std::map<std::string, VectorData> embeddings;
};

}  // namespace pipeline
}  // namespace ml
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ML_PIPELINE_PIPELINE_EMBEDDING_INFO_H_
