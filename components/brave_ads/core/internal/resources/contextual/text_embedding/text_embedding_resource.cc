/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_embedding/text_embedding_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
}  // namespace

TextEmbeddingResource::TextEmbeddingResource() = default;

TextEmbeddingResource::~TextEmbeddingResource() = default;

bool TextEmbeddingResource::IsInitialized() const {
  return embedding_processing_.IsInitialized();
}

void TextEmbeddingResource::Load() {
  LoadAndParseResource(
      kResourceId, kTextEmbeddingResourceVersion.Get(),
      base::BindOnce(&TextEmbeddingResource::OnLoadAndParseResource,
                     weak_factory_.GetWeakPtr()));
}

void TextEmbeddingResource::OnLoadAndParseResource(
    ResourceParsingErrorOr<ml::pipeline::EmbeddingProcessing> result) {
  if (!result.has_value()) {
    BLOG(1, result.error());
    BLOG(1,
         "Failed to initialize " << kResourceId << " text embedding resource");
    return;
  }
  BLOG(1, "Successfully loaded " << kResourceId << " text embedding resource");

  embedding_processing_ = std::move(result).value();

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " text embedding resource");
}

const ml::pipeline::EmbeddingProcessing* TextEmbeddingResource::Get() const {
  return &embedding_processing_;
}

}  // namespace brave_ads
