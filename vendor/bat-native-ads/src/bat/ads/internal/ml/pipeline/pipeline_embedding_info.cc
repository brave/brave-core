/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"

#include "bat/ads/internal/ml/ml_transformation_util.h"
#include "bat/ads/internal/ml/transformation/transformation.h"

namespace ads {
namespace ml {
namespace pipeline {

PipelineEmbeddingInfo::PipelineEmbeddingInfo() = default;

PipelineEmbeddingInfo::PipelineEmbeddingInfo(const PipelineEmbeddingInfo& info) {
  version = info.version;
  timestamp = info.timestamp;
  locale = info.locale;
  embeddings_dim = info.embeddings_dim;
  embeddings = info.embeddings;
}

PipelineEmbeddingInfo::~PipelineEmbeddingInfo() = default;

PipelineEmbeddingInfo::PipelineEmbeddingInfo(const int version,
                           const std::string& timestamp,
                           const std::string& locale,
                           const int embeddings_dim,
                           const std::map<std::string, VectorData>& embeddings)
    : version(version),
      timestamp(timestamp),
      locale(locale),
      embeddings_dim(embeddings_dim),
      embeddings(embeddings) {
  return;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
