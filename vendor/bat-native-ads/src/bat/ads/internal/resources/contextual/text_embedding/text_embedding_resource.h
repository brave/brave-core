/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/parsing_result.h"

namespace ads {

namespace ml::pipeline {
class EmbeddingProcessing;
}  // namespace ml::pipeline

namespace resource {

class TextEmbedding final {
 public:
  TextEmbedding();

  TextEmbedding(const TextEmbedding& other) = delete;
  TextEmbedding& operator=(const TextEmbedding& other) = delete;

  TextEmbedding(TextEmbedding&& other) noexcept = delete;
  TextEmbedding& operator=(TextEmbedding&& other) noexcept = delete;

  ~TextEmbedding();

  bool IsInitialized() const;

  void Load();

  ml::pipeline::EmbeddingProcessing* Get() const;

 private:
  void OnLoadAndParseResource(
      ParsingResultPtr<ml::pipeline::EmbeddingProcessing> result);

  std::unique_ptr<ml::pipeline::EmbeddingProcessing> embedding_processing_;

  base::WeakPtrFactory<TextEmbedding> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
