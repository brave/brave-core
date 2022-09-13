/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"

#include <utility>
#include <vector>

#include "base/time/time.h"

namespace ads::ml::pipeline {

namespace {

constexpr char kVersionKey[] = "version";
constexpr char kTimestampKey[] = "timestamp";
constexpr char kLocaleKey[] = "locale";
constexpr char kEmbeddingsKey[] = "embeddings";

}  // namespace

EmbeddingPipelineInfo::EmbeddingPipelineInfo() = default;

EmbeddingPipelineInfo::EmbeddingPipelineInfo(
    EmbeddingPipelineInfo&& info) noexcept = default;

EmbeddingPipelineInfo& EmbeddingPipelineInfo::operator=(
    EmbeddingPipelineInfo&& info) noexcept = default;

EmbeddingPipelineInfo::~EmbeddingPipelineInfo() = default;

bool EmbeddingPipelineInfo::FromValue(const base::Value::Dict& root) {
  if (absl::optional<int> value = root.FindInt(kVersionKey)) {
    version = *value;
  } else {
    return false;
  }

  if (const auto* value = root.FindString(kTimestampKey)) {
    if (!base::Time::FromUTCString((*value).c_str(), &time)) {
      return false;
    }
  }

  if (const auto* value = root.FindString(kLocaleKey)) {
    locale = *value;
  } else {
    return false;
  }

  const auto* value = root.FindDict(kEmbeddingsKey);
  if (!value) {
    return false;
  }

  dim = 1;
  for (const auto [key, value] : *value) {
    const auto* list = value.GetIfList();
    if (!list) {
      continue;
    }

    std::vector<float> embedding;
    embedding.reserve(list->size());
    for (const base::Value& v_raw : *list) {
      double v = v_raw.GetDouble();
      embedding.push_back(v);
    }
    embeddings[key] = VectorData(std::move(embedding));
    dim = embeddings[key].GetDimensionCount();
  }

  return dim != 1;
}

}  // namespace ads::ml::pipeline
