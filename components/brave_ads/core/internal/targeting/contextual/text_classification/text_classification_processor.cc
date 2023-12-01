/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"

#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::string GetTopSegmentFromPageProbabilities(
    const TextClassificationProbabilityMap& probabilities) {
  CHECK(!probabilities.empty());

  return base::ranges::max_element(probabilities,
                                   [](const SegmentProbabilityPair& lhs,
                                      const SegmentProbabilityPair& rhs) {
                                     return lhs.second < rhs.second;
                                   })
      ->first;
}

}  // namespace

TextClassificationProcessor::TextClassificationProcessor(
    TextClassificationResource& resource)
    : resource_(resource) {
  TabManager::GetInstance().AddObserver(this);
}

TextClassificationProcessor::~TextClassificationProcessor() {
  TabManager::GetInstance().RemoveObserver(this);
}

void TextClassificationProcessor::Process(const std::string& text) {
  if (!resource_->IsInitialized()) {
    return;
  }

  resource_->ClassifyPage(
      text, base::BindOnce(&TextClassificationProcessor::ClassifyPageCallback,
                           weak_factory_.GetWeakPtr()));
}

void TextClassificationProcessor::ClassifyPageCallback(
    const std::optional<TextClassificationProbabilityMap> probabilities) {
  if (!probabilities) {
    return BLOG(0, "Text classification failed due to an invalid model");
  }

  if (probabilities->empty()) {
    return BLOG(1, "Text not classified as not enough content");
  }

  const std::string top_segment =
      GetTopSegmentFromPageProbabilities(*probabilities);
  CHECK(!top_segment.empty());
  BLOG(1, "Classified text with the top segment as " << top_segment);

  ClientStateManager::GetInstance()
      .AppendTextClassificationProbabilitiesToHistory(*probabilities);
}

///////////////////////////////////////////////////////////////////////////////

void TextClassificationProcessor::OnTextContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (redirect_chain.empty()) {
    return;
  }

  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return BLOG(1,
                url.scheme()
                    << " scheme is not supported for processing text content");
  }

  if (IsSearchEngine(url) && !IsSearchEngineResultsPage(url)) {
    return BLOG(1,
                "Search engine landing pages are not supported for processing "
                "text content");
  }

  Process(text);
}

}  // namespace brave_ads
