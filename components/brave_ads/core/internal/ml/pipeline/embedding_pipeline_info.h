/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_INFO_H_

#include <map>
#include <string>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

namespace brave_ads::ml::pipeline {

struct EmbeddingPipelineInfo final {
  EmbeddingPipelineInfo();

  EmbeddingPipelineInfo(const EmbeddingPipelineInfo&) = delete;
  EmbeddingPipelineInfo& operator=(const EmbeddingPipelineInfo&) = delete;

  EmbeddingPipelineInfo(EmbeddingPipelineInfo&& other) noexcept;
  EmbeddingPipelineInfo& operator=(EmbeddingPipelineInfo&& other) noexcept;

  ~EmbeddingPipelineInfo();

  int version = 0;
  base::Time time;
  std::string locale;
  int dimension = 0;
  std::map<std::string, VectorData> embeddings;
};

}  // namespace brave_ads::ml::pipeline

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ML_PIPELINE_EMBEDDING_PIPELINE_INFO_H_
