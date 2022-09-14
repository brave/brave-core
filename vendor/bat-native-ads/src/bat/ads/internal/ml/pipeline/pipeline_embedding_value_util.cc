/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_value_util.h"

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"

#include <utility>
#include <vector>

namespace {

constexpr char kVersionKey[] = "version";
constexpr char kTimestampKey[] = "timestamp";
constexpr char kLocaleKey[] = "locale";
constexpr char kEmbeddingsKey[] = "embeddings";

}  // namespace

namespace ads::ml::pipeline {

absl::optional<EmbeddingPipelineInfo> EmbeddingPipelineFromValue(
    const base::Value::Dict& root) {
  EmbeddingPipelineInfo embedding_pipeline;

  if (absl::optional<int> value = root.FindInt(kVersionKey)) {
    embedding_pipeline.version = *value;
  } else {
    return absl::nullopt;
  }

  if (const auto* value = root.FindString(kTimestampKey)) {
    if (!base::Time::FromUTCString((*value).c_str(),
                                   &embedding_pipeline.time)) {
      return absl::nullopt;
    }
  }

  if (const auto* value = root.FindString(kLocaleKey)) {
    embedding_pipeline.locale = *value;
  } else {
    return absl::nullopt;
  }

  const auto* value = root.FindDict(kEmbeddingsKey);
  if (!value) {
    return absl::nullopt;
  }

  embedding_pipeline.dim = 1;
  for (const auto [key, value] : *value) {
    const auto* list = value.GetIfList();
    if (!list) {
      continue;
    }

    std::vector<float> embedding;
    embedding.reserve(list->size());
    for (const base::Value& dimension_value : *list) {
      embedding.push_back(dimension_value.GetDouble());
    }
    embedding_pipeline.embeddings[key] = VectorData(std::move(embedding));
    embedding_pipeline.dim =
        embedding_pipeline.embeddings[key].GetDimensionCount();
  }

  if (embedding_pipeline.dim == 1) {
    return absl::nullopt;
  }

  return embedding_pipeline;
}

}  // namespace ads::ml::pipeline
