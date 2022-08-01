/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_embedding_info.h"

#include <utility>

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

EmbeddingPipelineInfo::EmbeddingPipelineInfo(
    const int version,
    const base::Time timestamp,
    const std::string& locale,
    const int dim,
    const std::map<std::string, VectorData>& embeddings)
    : version(version),
      timestamp(timestamp),
      locale(locale),
      dim(dim),
      embeddings(embeddings) {}

absl::optional<EmbeddingPipelineInfo> ParseEmbeddingPipeline(
    base::Value value) {
  base::Value::Dict* resource = value.GetIfDict();
  if (!resource) {
    return absl::nullopt;
  }

  absl::optional<int> version_value = resource->FindInt("version");
  if (!version_value.has_value()) {
    return absl::nullopt;
  }

  std::string* timestamp_value = resource->FindString("timestamp");
  if (!timestamp_value) {
    return absl::nullopt;
  }
  base::Time timestamp;
  bool success =
      base::Time::FromUTCString((*timestamp_value).c_str(), &timestamp);
  if (!success) {
    return absl::nullopt;
  }

  std::string* locale_value = resource->FindString("locale");
  if (!locale_value) {
    return absl::nullopt;
  }

  base::Value::Dict* embeddings_value = resource->FindDict("embeddings");
  if (!embeddings_value) {
    return absl::nullopt;
  }
  int dim = 1;
  std::map<std::string, VectorData> embeddings;
  for (const auto item : *embeddings_value) {
    const auto vector = std::move(item.second.GetList());
    std::vector<float> embedding;
    embedding.reserve(vector.size());
    for (const base::Value& v_raw : vector) {
      double v = v_raw.GetDouble();
      embedding.push_back(v);
    }
    embeddings[item.first] = VectorData(std::move(embedding));
    dim = embeddings[item.first].GetDimensionCount();
  }

  absl::optional<EmbeddingPipelineInfo> pipeline_embedding =
    EmbeddingPipelineInfo(
      version_value.value(), 
      timestamp, 
      *locale_value, 
      dim, 
      embeddings
    );

  return pipeline_embedding;
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
