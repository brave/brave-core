/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include <iostream>
#include <algorithm>

#include "base/check.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/features/text_embedding_features.h"
#include "bat/ads/internal/locale/locale_manager.h"
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
    BLOG(1,
         "Failed to process token embeddings as resource not initialized");
    return;
  }

  ml::pipeline::EmbeddingProcessing* embedding_proc_pipeline = resource_->Get();

  const std::string cleaned_text = embedding_proc_pipeline->CleanText(text, true);
  if (cleaned_text.length() == 0) {
    BLOG(1, "No text available for embedding");
    return;
  }

  ml::VectorData text_embedding = embedding_proc_pipeline->EmbedText(cleaned_text);
  if (text_embedding.GetNonZeroElementsCount() == 0) {
    BLOG(1, "Text not embedded");
    return;
  }

  const std::string embedding_formatted = text_embedding.GetVectorAsString();

  std::cout << "\n\n";
  std::cout << embedding_formatted;
  std::cout << "\n\n";

  LogTextEmbeddingHTMLEvent(embedding_formatted, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to text embedding html event");
      return;
    }
    BLOG(1, "Successfully logged text embedding html event");
    std::cout << "\n\n";
    std::cout << "Successfully logged text embedding html event";
    std::cout << "\n";

    PurgeStaleTextEmbeddingHTMLEvents([](const bool success) {
      if (!success) {
        BLOG(1, "Failed to purge stale text embedding html events");
        return;
      }
      BLOG(1, "Successfully purged stale text embedding html events");
      std::cout << "Successfully purged stale text embedding html events";
      std::cout << "\n";
    });

    std::cout << "\n";
    GetTextEmbeddingEventsFromDatabase();
    std::cout << "\n";

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

  // const GURL& url = redirect_chain.back();

  // if (!url.SchemeIsHTTPOrHTTPS()) {
  //   BLOG(
  //       1,
  //       url.scheme() << " scheme is not supported for processing html");
  //   return;
  // }

  // if (IsSearchEngine(url) && !IsSearchEngineResultsPage(url)) {
  //   BLOG(1,
  //        "Search engine landing pages are not supported for processing html");
  //   return;
  // }

  if (TextEmbedding::IsEmbeddingEnabled()) { 
    Process(html);
  }
}

}  // namespace processor
}  // namespace ads
