/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_error_or.h"

namespace brave_ads {

class TextEmbeddingResource final {
 public:
  TextEmbeddingResource();

  TextEmbeddingResource(const TextEmbeddingResource&) = delete;
  TextEmbeddingResource& operator=(const TextEmbeddingResource&) = delete;

  TextEmbeddingResource(TextEmbeddingResource&&) noexcept = delete;
  TextEmbeddingResource& operator=(TextEmbeddingResource&&) noexcept = delete;

  ~TextEmbeddingResource();

  bool IsInitialized() const;

  void Load();

  const ml::pipeline::EmbeddingProcessing* Get() const;

 private:
  void OnLoadAndParseResource(
      ResourceParsingErrorOr<ml::pipeline::EmbeddingProcessing> result);

  ml::pipeline::EmbeddingProcessing embedding_processing_;

  base::WeakPtrFactory<TextEmbeddingResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
