/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/features/text_embedding_features.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "bat/ads/internal/resources/resources_util_impl.h"

namespace ads::resource {

namespace {
constexpr char kResourceId[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
}  // namespace

TextEmbedding::TextEmbedding()
    : embedding_processing_(
          std::make_unique<ml::pipeline::EmbeddingProcessing>()) {}

TextEmbedding::~TextEmbedding() = default;

bool TextEmbedding::IsInitialized() const {
  return embedding_processing_ && embedding_processing_->IsInitialized();
}

void TextEmbedding::Load() {
  LoadAndParseResource(kResourceId,
                       targeting::features::GetTextEmbeddingResourceVersion(),
                       base::BindOnce(&TextEmbedding::OnLoadAndParseResource,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void TextEmbedding::OnLoadAndParseResource(
    ParsingResultPtr<ml::pipeline::EmbeddingProcessing> result) {
  if (!result) {
    BLOG(1, "Failed to load " << kResourceId << " text embedding resource");
    return;
  }
  BLOG(1, "Successfully loaded " << kResourceId << " text embedding resource");

  if (!result->resource) {
    BLOG(1, result->error_message);
    BLOG(1,
         "Failed to initialize " << kResourceId << " text embedding resource");
    return;
  }

  embedding_processing_ = std::move(result->resource);

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " text embedding resource");
}

ml::pipeline::EmbeddingProcessing* TextEmbedding::Get() const {
  return embedding_processing_.get();
}

}  // namespace ads::resource
