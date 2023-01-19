/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/embedding_pipeline_info.h"

namespace ads::ml::pipeline {

EmbeddingPipelineInfo::EmbeddingPipelineInfo() = default;

EmbeddingPipelineInfo::EmbeddingPipelineInfo(
    const EmbeddingPipelineInfo& other) = default;

EmbeddingPipelineInfo& EmbeddingPipelineInfo::operator=(
    const EmbeddingPipelineInfo& other) = default;

EmbeddingPipelineInfo::EmbeddingPipelineInfo(
    EmbeddingPipelineInfo&& other) noexcept = default;

EmbeddingPipelineInfo& EmbeddingPipelineInfo::operator=(
    EmbeddingPipelineInfo&& other) noexcept = default;

EmbeddingPipelineInfo::~EmbeddingPipelineInfo() = default;

}  // namespace ads::ml::pipeline
