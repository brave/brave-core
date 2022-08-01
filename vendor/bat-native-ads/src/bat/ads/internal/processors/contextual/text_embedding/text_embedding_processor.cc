/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include "base/check.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/search_engine/search_engine_results_page_util.h"
#include "bat/ads/internal/base/search_engine/search_engine_util.h"
#include "bat/ads/internal/features/text_embedding_features.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_data.h"
#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "bat/ads/internal/resources/language_components.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "url/gurl.h"

namespace ads {
namespace processor {

TextEmbedding::TextEmbedding(resource::TextEmbedding* resource)
    : resource_(resource) {
  DCHECK(resource_);

  LocaleManager::GetInstance()->AddObserver(this);
  ResourceManager::GetInstance()->AddObserver(this);
  TabManager::GetInstance()->AddObserver(this);
}

TextEmbedding::~TextEmbedding() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  ResourceManager::GetInstance()->RemoveObserver(this);
  TabManager::GetInstance()->RemoveObserver(this);
}

bool TextEmbedding::IsEmbeddingEnabled() {
  return targeting::features::IsTextEmbeddingEnabled();
}

void TextEmbedding::Process(const std::string& text) {
  if (!resource_->IsInitialized()) {
    BLOG(1, "Failed to process token embeddings as resource not initialized");
    return;
  }

  const std::string sanitized_text = SanitizeText(text, true);
  if (sanitized_text.length() == 0) {
    BLOG(1, "No text available for embedding");
    return;
  }

  ml::pipeline::EmbeddingProcessing* embedding_proc_pipeline = resource_->Get();
  ml::pipeline::TextEmbeddingData text_embedding_data =
      embedding_proc_pipeline->EmbedText(sanitized_text);
  if (text_embedding_data.embedding.GetNonZeroElementsCount() == 0) {
    BLOG(1, "Failed to create text embedding");
    return;
  }

  const std::string embedding_formatted =
      text_embedding_data.embedding.GetVectorAsString();
  BLOG(9, "Embedding: " << embedding_formatted);
  LogTextEmbeddingHtmlEvent(
      embedding_formatted, text_embedding_data.text_hashed,
      [](const bool success) {
        if (!success) {
          BLOG(1, "Failed to log text embedding html event");
          return;
        }

        PurgeStaleTextEmbeddingHtmlEvents([](const bool success) {
          if (!success) {
            BLOG(1, "Failed to purge stale text embedding html events");
            return;
          }
        });
      });
}

///////////////////////////////////////////////////////////////////////////////

void TextEmbedding::OnLocaleDidChange(const std::string& locale) {
  resource_->Load();
}

void TextEmbedding::OnResourceDidUpdate(const std::string& id) {
  if (kLanguageComponentIds.find(id) != kLanguageComponentIds.end()) {
    resource_->Load();
  }
}

void TextEmbedding::OnHtmlContentDidChange(
    const int32_t id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    BLOG(1, url.scheme() << " scheme is not supported for processing html");
    return;
  }

  if (IsSearchEngine(url) && !IsSearchEngineResultsPage(url)) {
    BLOG(1,
         "Search engine landing pages are not supported for processing html");
    return;
  }

  if (TextEmbedding::IsEmbeddingEnabled()) {
    Process(html);
  }
}

}  // namespace processor
}  // namespace ads
