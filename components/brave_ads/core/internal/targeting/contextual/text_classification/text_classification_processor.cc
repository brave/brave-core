/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
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
  if (resource_->IsLoaded()) {
    const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
        kTraceEventCategory, "TextClassificationProcessor::Process",
        TRACE_ID_WITH_SCOPE("TextClassificationProcessor", trace_id));

    resource_->ClassifyPage(
        text, base::BindOnce(&TextClassificationProcessor::ClassifyPageCallback,
                             weak_factory_.GetWeakPtr(), trace_id));
  }
}

void TextClassificationProcessor::ClassifyPageCallback(
    uint64_t trace_id,
    base::optional_ref<const TextClassificationProbabilityMap> probabilities) {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      kTraceEventCategory, "TextClassificationProcessor::Process",
      TRACE_ID_WITH_SCOPE("TextClassificationProcessor", trace_id));

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
    int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  CHECK(!redirect_chain.empty());

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
