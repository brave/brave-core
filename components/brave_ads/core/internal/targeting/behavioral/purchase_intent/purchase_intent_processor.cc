/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_parser.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_info.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
constexpr int kDefaultFunnelKeyphraseWeightForSearchQuery = 1;
}  // namespace

PurchaseIntentProcessor::PurchaseIntentProcessor(
    PurchaseIntentResource& resource)
    : resource_(resource) {
  TabManager::GetInstance().AddObserver(this);
}

PurchaseIntentProcessor::~PurchaseIntentProcessor() {
  TabManager::GetInstance().RemoveObserver(this);
}

void PurchaseIntentProcessor::Process(const GURL& url) {
  if (!resource_->IsLoaded()) {
    return;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return BLOG(
        1, url.scheme()
               << " scheme is not supported for processing purchase intent");
  }

  if (!url.is_valid()) {
    return BLOG(
        1,
        "Failed to process purchase intent because the visited URL is invalid");
  }

  const std::optional<PurchaseIntentSignalInfo> signal =
      MaybeExtractSignal(url);
  if (!signal || signal->segments.empty()) {
    return BLOG(1, "No purchase intent matches found");
  }

  BuyPurchaseIntentSignal(*signal);
}

///////////////////////////////////////////////////////////////////////////////

bool PurchaseIntentProcessor::ShouldProcess(const int32_t tab_id,
                                            const GURL& url) const {
  const auto iter = tabs_.find(tab_id);
  if (iter == tabs_.cend()) {
    return true;
  }

  return iter->second != url;
}

void PurchaseIntentProcessor::MaybeProcess(const int32_t tab_id,
                                           const GURL& url) {
  if (!ShouldProcess(tab_id, url)) {
    return;
  }

  tabs_[tab_id] = url;

  Process(url);
}

std::optional<PurchaseIntentSignalInfo>
PurchaseIntentProcessor::MaybeExtractSignal(const GURL& url) const {
  const std::optional<std::string> search_term_query_value =
      ExtractSearchTermQueryValue(url);

  return search_term_query_value
             ? MaybeExtractSignalForSearchQuery(*search_term_query_value)
             : MaybeExtractSignalForUrl(url);
}

std::optional<PurchaseIntentSignalInfo>
PurchaseIntentProcessor::MaybeExtractSignalForSearchQuery(
    const std::string& search_query) const {
  BLOG(1, "Extracting purchase intent signal from search query");

  KeywordList search_query_keywords = ParseKeyphrase(search_query);
  base::ranges::sort(search_query_keywords);

  const std::optional<SegmentList> search_query_segments =
      MaybeGetSegmentsForSearchQuery(search_query_keywords);
  if (!search_query_segments || search_query_segments->empty()) {
    return std::nullopt;
  }

  BLOG(1, "Extracted purchase intent signal from search query");

  return PurchaseIntentSignalInfo(
      /*at=*/base::Time::Now(), *search_query_segments,
      ComputeFunnelKeyphraseWeightForSearchQuery(search_query_keywords));
}

std::optional<SegmentList>
PurchaseIntentProcessor::MaybeGetSegmentsForSearchQuery(
    const KeywordList& search_query_keywords) const {
  const std::optional<PurchaseIntentResourceInfo>& purchase_intent =
      resource_->get();
  if (!purchase_intent) {
    return std::nullopt;
  }

  const auto iter = base::ranges::find_if(
      purchase_intent->segment_keyphrases,
      [&search_query_keywords](const auto& segment_keyphrase) {
        return base::ranges::includes(search_query_keywords,
                                      segment_keyphrase.keywords);
      });

  if (iter == purchase_intent->segment_keyphrases.cend()) {
    return std::nullopt;
  }

  return iter->segments;
}

int PurchaseIntentProcessor::ComputeFunnelKeyphraseWeightForSearchQuery(
    const KeywordList& search_query_keywords) const {
  const std::optional<PurchaseIntentResourceInfo>& purchase_intent =
      resource_->get();
  if (!purchase_intent) {
    return kDefaultFunnelKeyphraseWeightForSearchQuery;
  }

  int max_funnel_keyphrase_weight = kDefaultFunnelKeyphraseWeightForSearchQuery;

  for (const auto& funnel_keyphrase : purchase_intent->funnel_keyphrases) {
    if (funnel_keyphrase.weight > max_funnel_keyphrase_weight &&
        base::ranges::includes(search_query_keywords,
                               funnel_keyphrase.keywords)) {
      max_funnel_keyphrase_weight = funnel_keyphrase.weight;
    }
  }

  return max_funnel_keyphrase_weight;
}

std::optional<PurchaseIntentSignalInfo>
PurchaseIntentProcessor::MaybeExtractSignalForUrl(const GURL& url) const {
  BLOG(1, "Extracting purchase intent signal from visited URL");

  const std::optional<PurchaseIntentFunnelInfo> funnel =
      MaybeGetFunnelForUrl(url);
  if (!funnel) {
    return std::nullopt;
  }

  BLOG(1, "Extracted purchase intent signal from visited URL");

  return PurchaseIntentSignalInfo(
      /*at=*/base::Time::Now(), funnel->segments, funnel->weight);
}

std::optional<PurchaseIntentFunnelInfo>
PurchaseIntentProcessor::MaybeGetFunnelForUrl(const GURL& url) const {
  const std::optional<PurchaseIntentResourceInfo>& purchase_intent =
      resource_->get();
  if (!purchase_intent) {
    return std::nullopt;
  }

  const auto iter =
      purchase_intent->funnel_sites.find(url.GetWithEmptyPath().spec());
  if (iter == purchase_intent->funnel_sites.cend()) {
    return std::nullopt;
  }

  return iter->second;
}

void PurchaseIntentProcessor::OnDidOpenNewTab(const TabInfo& tab) {
  CHECK(!tab.redirect_chain.empty());

  const GURL& url = tab.redirect_chain.back();
  MaybeProcess(tab.id, url);
}

void PurchaseIntentProcessor::OnTabDidChange(const TabInfo& tab) {
  CHECK(!tab.redirect_chain.empty());

  const GURL& url = tab.redirect_chain.back();
  MaybeProcess(tab.id, url);
}

void PurchaseIntentProcessor::OnDidCloseTab(const int32_t tab_id) {
  tabs_.erase(tab_id);
}

}  // namespace brave_ads
