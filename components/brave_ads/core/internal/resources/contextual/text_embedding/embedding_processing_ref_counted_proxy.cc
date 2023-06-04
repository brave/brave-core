/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/embedding_processing_ref_counted_proxy.h"

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"

namespace brave_ads {

EmbeddingProcessingRefCountedProxy::EmbeddingProcessingRefCountedProxy() =
    default;

EmbeddingProcessingRefCountedProxy::~EmbeddingProcessingRefCountedProxy() =
    default;

ml::pipeline::TextEmbeddingInfo EmbeddingProcessingRefCountedProxy::EmbedText(
    const std::string& text) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const absl::optional<ml::pipeline::EmbeddingProcessing>& resource =
      GetResource();

  if (!resource || !resource->IsInitialized()) {
    return {};
  }

  ml::pipeline::TextEmbeddingInfo text_embedding_info =
      resource->EmbedText(text);
  return text_embedding_info;
}

}  // namespace brave_ads
