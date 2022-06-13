/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/parsing_result.h"
#include "bat/ads/internal/resources/resource_interface.h"

namespace ads {

namespace ml {
namespace pipeline {
class EmbeddingProcessing;
}  // namespace pipeline
}  // namespace ml

namespace resource {

class TextEmbedding final
    : public ResourceInterface<ml::pipeline::EmbeddingProcessing*> {
 public:
  TextEmbedding();
  ~TextEmbedding() override;

  TextEmbedding(const TextEmbedding&) = delete;
  TextEmbedding& operator=(const TextEmbedding&) = delete;

  bool IsInitialized() const override;

  void Load();

  ml::pipeline::EmbeddingProcessing* get() const override;

 private:
  void OnLoadAndParseResource(
      ParsingResultPtr<ml::pipeline::EmbeddingProcessing> result);

  std::unique_ptr<ml::pipeline::EmbeddingProcessing> embedding_processing_pipeline_;

  base::WeakPtrFactory<TextEmbedding> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_RESOURCE_H_
