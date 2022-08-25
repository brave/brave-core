/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"

#include <utility>
#include <vector>

#include "base/time/time.h"

namespace ads {
namespace ml {
namespace pipeline {

EmbeddingPipelineInfo::EmbeddingPipelineInfo() = default;

EmbeddingPipelineInfo::EmbeddingPipelineInfo(
    EmbeddingPipelineInfo&& info) noexcept = default;

EmbeddingPipelineInfo& EmbeddingPipelineInfo::operator=(
    EmbeddingPipelineInfo&& info) noexcept = default;

EmbeddingPipelineInfo::~EmbeddingPipelineInfo() = default;

bool EmbeddingPipelineInfo::FromValue(base::Value::Dict& root) {
  if (absl::optional<int> value = root.FindInt("version")) {
    version = value.value();
  }
  else {
    return false;
  }

  if (const auto* value = root.FindString("timestamp")) {
    if (!base::Time::FromUTCString((*value).c_str(), &time)) {
      return false;
    }
  }

  if (const auto* value = root.FindString("locale")) {
    locale = *value;
  }
  else {
    return false;
  }

  if (auto* value = root.FindDict("embeddings")) {
    dim = 1;
    for (const auto item : *value) {
      const auto list = std::move(item.second.GetList());
      std::vector<float> embedding;
      embedding.reserve(list.size());
      for (const base::Value& v_raw : list) {
        double v = v_raw.GetDouble();
        embedding.push_back(v);
      }
      embeddings[item.first] = VectorData(std::move(embedding));
      dim = embeddings[item.first].GetDimensionCount();
    }

    if (dim == 1) {
      return false;
    }
  }
  else {
    return false;
  }

  return true;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
