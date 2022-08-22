/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_info.h"

namespace ads {
namespace ml {
namespace pipeline {

TextEmbeddingInfo::TextEmbeddingInfo() = default;
TextEmbeddingInfo::TextEmbeddingInfo(const TextEmbeddingInfo& info) = default;
TextEmbeddingInfo& TextEmbeddingInfo::operator=(const TextEmbeddingInfo& info) =
    default;
TextEmbeddingInfo::~TextEmbeddingInfo() = default;

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
