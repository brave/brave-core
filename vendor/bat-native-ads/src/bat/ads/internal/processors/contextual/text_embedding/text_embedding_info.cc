/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_info.h"

namespace ads {

TextEmbeddingInfo::TextEmbeddingInfo() = default;

TextEmbeddingInfo::TextEmbeddingInfo(const TextEmbeddingInfo& info) = default;

TextEmbeddingInfo& TextEmbeddingInfo::operator=(const TextEmbeddingInfo& info) = default;

TextEmbeddingInfo::~TextEmbeddingInfo() = default;

bool TextEmbeddingInfo::operator==(const TextEmbeddingInfo& rhs) const {
  return version == rhs.version && locale == rhs.locale && embedding == rhs.embedding;
}

bool TextEmbeddingInfo::operator!=(const TextEmbeddingInfo& rhs) const {
  return !(*this == rhs);
}

bool TextEmbeddingInfo::IsValid() const {
  if (embedding.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
