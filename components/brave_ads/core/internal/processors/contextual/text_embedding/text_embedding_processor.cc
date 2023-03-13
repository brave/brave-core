/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_processor.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_util.h"
#include "brave/components/brave_ads/core/internal/features/text_embedding_features.h"
#include "brave/components/brave_ads/core/internal/locale/locale_manager.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_processing.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events.h"
#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"
#include "brave/components/brave_ads/core/internal/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/resources/resource_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "url/gurl.h"

namespace brave_ads::processor {

namespace {

bool IsEnabled() {
  return targeting::features::IsTextEmbeddingEnabled();
}

}  // namespace

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

void TextEmbedding::Process(const std::string& html) {
  if (!resource_->IsInitialized()) {
    BLOG(1, "Failed to process text embeddings as resource not initialized");
    return;
  }

  const std::string text = SanitizeHtml(html);
  if (text.empty()) {
    BLOG(1, "No text available for embedding");
    return;
  }

  const ml::pipeline::EmbeddingProcessing* const embedding_proc_pipeline =
      resource_->Get();
  const ml::pipeline::TextEmbeddingInfo text_embedding =
      embedding_proc_pipeline->EmbedText(text);
  if (text_embedding.embedding.GetNonZeroElementCount() == 0) {
    BLOG(1, "Failed to embed text");
    return;
  }

  LogTextEmbeddingHtmlEvent(
      BuildTextEmbeddingHtmlEvent(text_embedding),
      base::BindOnce([](const bool success) {
        if (!success) {
          BLOG(1, "Failed to log text embedding HTML event");
          return;
        }

        BLOG(3, "Successfully logged text embedding HTML event");

        PurgeStaleTextEmbeddingHtmlEvents(
            base::BindOnce([](const bool success) {
              if (!success) {
                BLOG(1, "Failed to purge stale text embedding HTML events");
                return;
              }

              BLOG(3, "Successfully purged stale text embedding HTML events");
            }));
      }));
}

///////////////////////////////////////////////////////////////////////////////

void TextEmbedding::OnLocaleDidChange(const std::string& /*locale*/) {
  resource_->Load();
}

void TextEmbedding::OnResourceDidUpdate(const std::string& id) {
  if (IsValidLanguageComponentId(id)) {
    resource_->Load();
  }
}

void TextEmbedding::OnHtmlContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (redirect_chain.empty()) {
    return;
  }

  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    BLOG(
        1,
        url.scheme() << " scheme is not supported for processing HTML content");
    return;
  }

  if (IsSearchEngine(url) && !IsSearchEngineResultsPage(url)) {
    BLOG(1,
         "Search engine landing pages are not supported for processing HTML "
         "content");
    return;
  }

  if (!IsEnabled()) {
    return;
  }

  Process(html);
}

}  // namespace brave_ads::processor
