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

bool EmbeddingPipelineInfo::FromValue(base::Value::Dict& value) {
  absl::optional<int> version_value = value.FindInt("version");
  if (!version_value) {
    return false;
  }
  version = version_value.value();

  std::string* timestamp_value = value.FindString("timestamp");
  if (!timestamp_value) {
    return false;
  }
  base::Time time;
  if (!base::Time::FromUTCString((*timestamp_value).c_str(), &time)) {
    return false;
  }

  std::string* locale_value = value.FindString("locale");
  if (!locale_value) {
    return false;
  }
  locale = *locale_value;

  base::Value::Dict* embeddings_value = value.FindDict("embeddings");
  if (!embeddings_value) {
    return false;
  }

  dim = 1;
  for (const auto item : *embeddings_value) {
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

  return true;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
