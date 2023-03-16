/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"

namespace brave_ads::ml::pipeline {

TextEmbeddingInfo::TextEmbeddingInfo() = default;

TextEmbeddingInfo::TextEmbeddingInfo(const TextEmbeddingInfo& other) = default;

TextEmbeddingInfo& TextEmbeddingInfo::operator=(
    const TextEmbeddingInfo& other) = default;

TextEmbeddingInfo::TextEmbeddingInfo(TextEmbeddingInfo&& other) noexcept =
    default;

TextEmbeddingInfo& TextEmbeddingInfo::operator=(
    TextEmbeddingInfo&& other) noexcept = default;

TextEmbeddingInfo::~TextEmbeddingInfo() = default;

}  // namespace brave_ads::ml::pipeline
