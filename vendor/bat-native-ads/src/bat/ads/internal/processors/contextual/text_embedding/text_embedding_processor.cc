/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor.h"

#include <iostream>
#include <algorithm>

#include "base/check.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "bat/ads/internal/serving/targeting/models/contextual/text_embedding/text_embedding_aliases.h"

namespace ads {
namespace processor {

TextEmbedding::TextEmbedding(resource::TextEmbedding* resource)
    : resource_(resource) {
  DCHECK(resource_);
}

TextEmbedding::~TextEmbedding() = default;

void TextEmbedding::Process(const std::string& text) {
  if (!resource_->IsInitialized()) {
    BLOG(1,
         "Failed to process token embeddings as resource "
         "not initialized");
    return;
  }

  ml::pipeline::EmbeddingProcessing* embedding_proc_pipeline = resource_->get();

  const std::string cleaned_text = embedding_proc_pipeline->CleanText(text, true);
  if (cleaned_text.length() == 0) {
    BLOG(1, "No text available for embedding");
    return;
  }

  ml::VectorData text_embedding = embedding_proc_pipeline->EmbedText(cleaned_text);
  if (text_embedding.VectorSumElements() == 0.0) {
    BLOG(1, "Text not embedded");
    return;
  }

  std::cout << "\n\n";
  std::cout << text_embedding.GetVectorAsString();
  std::cout << "\n\n";

  ClientStateManager::Get()->AppendTextEmbeddingToHistory(text_embedding);

  std::cout << "checking embedding stored history:";
  const targeting::TextEmbeddingList& embedding_history = ClientStateManager::Get()->GetTextEmbeddingHistory();
  for (const auto& embedding : embedding_history) {
    std::cout << "\n\n";
    std::cout << embedding.GetVectorAsString();
  }
}

}  // namespace processor
}  // namespace ads
