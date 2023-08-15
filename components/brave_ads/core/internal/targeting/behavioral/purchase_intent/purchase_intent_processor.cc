/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_site_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

using KeywordList = std::vector<std::string>;

namespace {

constexpr uint16_t kPurchaseIntentDefaultSignalWeight = 1;

void AppendSignalToHistory(
    const PurchaseIntentSignalInfo& purchase_intent_signal) {
  for (const auto& segment : purchase_intent_signal.segments) {
    const PurchaseIntentSignalHistoryInfo history(
        purchase_intent_signal.created_at, purchase_intent_signal.weight);

    ClientStateManager::GetInstance()
        .AppendToPurchaseIntentSignalHistoryForSegment(segment, history);
  }
}

KeywordList ToKeywords(const std::string& value) {
  return base::SplitString(
      StripNonAlphaNumericCharacters(base::ToLowerASCII(value)), " ",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

bool IsSubset(KeywordList keywords_lhs, KeywordList keywords_rhs) {
  base::ranges::sort(keywords_lhs);
  base::ranges::sort(keywords_rhs);
  return base::ranges::includes(keywords_lhs, keywords_rhs);
}

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
  if (!resource_->IsInitialized()) {
    return;
  }

  if (!url.is_valid()) {
    return BLOG(1,
                "Failed to process purchase intent signal because the visited "
                "URL is invalid");
  }

  const absl::optional<PurchaseIntentSignalInfo> signal = ExtractSignal(url);
  if (!signal || signal->segments.empty()) {
    return BLOG(1, "No purchase intent matches found for visited URL");
  }

  BLOG(1, "Extracted purchase intent signal from visited URL");

  AppendSignalToHistory(*signal);
}

///////////////////////////////////////////////////////////////////////////////

absl::optional<PurchaseIntentSignalInfo> PurchaseIntentProcessor::ExtractSignal(
    const GURL& url) const {
  const absl::optional<std::string> search_query =
      ExtractSearchTermQueryValue(url);
  if (search_query) {
    const absl::optional<SegmentList> segments =
        GetSegmentsForSearchQuery(*search_query);
    if (!segments || segments->empty()) {
      return absl::nullopt;
    }

    PurchaseIntentSignalInfo purchase_intent_signal;
    purchase_intent_signal.created_at = base::Time::Now();
    purchase_intent_signal.segments = *segments;
    purchase_intent_signal.weight =
        GetFunnelWeightForSearchQuery(*search_query);
    return purchase_intent_signal;
  }

  const absl::optional<PurchaseIntentSiteInfo> site = GetSite(url);
  if (!site || !site->url_netloc.is_valid()) {
    return absl::nullopt;
  }

  PurchaseIntentSignalInfo purchase_intent_signal;
  purchase_intent_signal.created_at = base::Time::Now();
  purchase_intent_signal.segments = site->segments;
  purchase_intent_signal.weight = site->weight;
  return purchase_intent_signal;
}

absl::optional<PurchaseIntentSiteInfo> PurchaseIntentProcessor::GetSite(
    const GURL& url) const {
  const absl::optional<PurchaseIntentInfo>& purchase_intent = resource_->get();
  if (!purchase_intent) {
    return absl::nullopt;
  }

  for (const auto& site : purchase_intent->sites) {
    if (SameDomainOrHost(url, site.url_netloc)) {
      return site;
    }
  }

  return absl::nullopt;
}

absl::optional<SegmentList> PurchaseIntentProcessor::GetSegmentsForSearchQuery(
    const std::string& search_query) const {
  const absl::optional<PurchaseIntentInfo>& purchase_intent = resource_->get();
  if (!purchase_intent) {
    return absl::nullopt;
  }

  const KeywordList search_query_keywords = ToKeywords(search_query);

  for (const auto& keyword : purchase_intent->segment_keywords) {
    // Intended behavior relies on early return from list traversal and
    // implicitely on the ordering of |segment_keywords_| to ensure specific
    // segments are matched over general segments, e.g. "audi a6" segments
    // should be returned over "audi" segments if possible.
    if (IsSubset(search_query_keywords, ToKeywords(keyword.keywords))) {
      return keyword.segments;
    }
  }

  return absl::nullopt;
}

uint16_t PurchaseIntentProcessor::GetFunnelWeightForSearchQuery(
    const std::string& search_query) const {
  const absl::optional<PurchaseIntentInfo>& purchase_intent = resource_->get();
  if (!purchase_intent) {
    return kPurchaseIntentDefaultSignalWeight;
  }

  const KeywordList search_query_keywords = ToKeywords(search_query);

  uint16_t max_weight = kPurchaseIntentDefaultSignalWeight;

  for (const auto& keyword : purchase_intent->funnel_keywords) {
    const KeywordList keywords = ToKeywords(keyword.keywords);

    if (keyword.weight > max_weight &&
        IsSubset(search_query_keywords, keywords)) {
      max_weight = keyword.weight;
    }
  }

  return max_weight;
}

void PurchaseIntentProcessor::OnTextContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& /*text*/) {
  if (redirect_chain.empty()) {
    return;
  }

  const GURL& url = redirect_chain.back();

  if (!url.SchemeIsHTTPOrHTTPS()) {
    BLOG(1, url.scheme()
                << " scheme is not supported for processing purchase intent");
    return;
  }

  const absl::optional<TabInfo> last_visible_tab =
      TabManager::GetInstance().GetLastVisible();
  if (!last_visible_tab) {
    return;
  }

  if (last_visible_tab->redirect_chain.empty()) {
    return;
  }

  if (SameDomainOrHost(url, last_visible_tab->redirect_chain.back())) {
    return;
  }

  Process(url);
}

}  // namespace brave_ads
