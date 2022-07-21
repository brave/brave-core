/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"

namespace ads {
namespace ml {
namespace pipeline {

TextEmbeddingData::TextEmbeddingData() = default;
TextEmbeddingData::TextEmbeddingData(TextEmbeddingData& data) = default;
TextEmbeddingData& TextEmbeddingData::operator=(const TextEmbeddingData& data) = default;
TextEmbeddingData::~TextEmbeddingData() = default;

}  // namespace pipeline
}  // namespace ml
}  // namespace ads