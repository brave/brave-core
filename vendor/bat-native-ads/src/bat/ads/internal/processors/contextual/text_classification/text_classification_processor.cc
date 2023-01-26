/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_classification/text_classification_processor.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/search_engine/search_engine_results_page_util.h"
#include "bat/ads/internal/common/search_engine/search_engine_util.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"
#include "bat/ads/internal/resources/language_components.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "url/gurl.h"

namespace ads::processor {

namespace {

std::string GetTopSegmentFromPageProbabilities(
    const targeting::TextClassificationProbabilityMap& probabilities) {
  DCHECK(!probabilities.empty());

  return base::ranges::max_element(
             probabilities,
             [](const targeting::SegmentProbabilityPair& lhs,
                const targeting::SegmentProbabilityPair& rhs) {
               return lhs.second < rhs.second;
             })
      ->first;
}

}  // namespace

TextClassification::TextClassification(resource::TextClassification* resource)
    : resource_(resource) {
  DCHECK(resource_);

  LocaleManager::GetInstance()->AddObserver(this);
  ResourceManager::GetInstance()->AddObserver(this);
  TabManager::GetInstance()->AddObserver(this);
}

TextClassification::~TextClassification() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  ResourceManager::GetInstance()->RemoveObserver(this);
  TabManager::GetInstance()->RemoveObserver(this);
}

void TextClassification::Process(const std::string& text) {
  if (!resource_->IsInitialized()) {
    BLOG(1,
         "Failed to process text classification as resource not initialized");
    return;
  }

  const ml::pipeline::TextProcessing* const text_proc_pipeline =
      resource_->Get();

  const targeting::TextClassificationProbabilityMap probabilities =
      text_proc_pipeline->ClassifyPage(text);

  if (probabilities.empty()) {
    BLOG(1, "Text not classified as not enough content");
    return;
  }

  const std::string segment = GetTopSegmentFromPageProbabilities(probabilities);
  DCHECK(!segment.empty());
  BLOG(1, "Classified text with the top segment as " << segment);

  ClientStateManager::GetInstance()
      ->AppendTextClassificationProbabilitiesToHistory(probabilities);
}

///////////////////////////////////////////////////////////////////////////////

void TextClassification::OnLocaleDidChange(const std::string& /*locale*/) {
  resource_->Load();
}

void TextClassification::OnResourceDidUpdate(const std::string& id) {
  if (IsValidLanguageComponentId(id)) {
    resource_->Load();
  }
}

void TextClassification::OnTextContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  if (redirect_chain.empty()) {
    return;
  }

  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    BLOG(
        1,
        url.scheme() << " scheme is not supported for processing text content");
    return;
  }

  if (IsSearchEngine(url) && !IsSearchEngineResultsPage(url)) {
    BLOG(1,
         "Search engine landing pages are not supported for processing text "
         "content");
    return;
  }

  Process(content);
}

}  // namespace ads::processor
