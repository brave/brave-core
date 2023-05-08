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
#include "brave/components/brave_ads/core/internal/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
}  // namespace

TextEmbeddingResource::TextEmbeddingResource() {
  AdsClientHelper::AddObserver(this);
}

TextEmbeddingResource::~TextEmbeddingResource() {
  AdsClientHelper::RemoveObserver(this);
}

bool TextEmbeddingResource::IsInitialized() const {
  return embedding_processing_.IsInitialized();
}

void TextEmbeddingResource::Load() {
  LoadAndParseResource(
      kResourceId, kTextEmbeddingResourceVersion.Get(),
      base::BindOnce(&TextEmbeddingResource::LoadAndParseResourceCallback,
                     weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void TextEmbeddingResource::LoadAndParseResourceCallback(
    ResourceParsingErrorOr<ml::pipeline::EmbeddingProcessing> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize "
                << kResourceId << " text embedding resource (" << result.error()
                << ")");
    return;
  }

  if (!result.value().IsInitialized()) {
    BLOG(7, kResourceId << " text embedding resource does not exist");
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId << " text embedding resource");

  embedding_processing_ = std::move(result).value();

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " text embedding resource version "
                                      << kTextEmbeddingResourceVersion.Get());
}

void TextEmbeddingResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  Load();
}

void TextEmbeddingResource::OnNotifyDidUpdateResourceComponent(
    const std::string& id) {
  if (IsValidLanguageComponentId(id)) {
    Load();
  }
}

}  // namespace brave_ads
