/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/purchase_intent/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_site_info.h"
#include "brave/components/brave_ads/core/internal/resources/country_components.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads::processor {

using KeywordList = std::vector</*keyword*/ std::string>;

namespace {

constexpr uint16_t kPurchaseIntentDefaultSignalWeight = 1;

void AppendIntentSignalToHistory(
    const targeting::PurchaseIntentSignalInfo& purchase_intent_signal) {
  for (const auto& segment : purchase_intent_signal.segments) {
    const targeting::PurchaseIntentSignalHistoryInfo history(
        purchase_intent_signal.created_at, purchase_intent_signal.weight);

    ClientStateManager::GetInstance()
        .AppendToPurchaseIntentSignalHistoryForSegment(segment, history);
  }
}

KeywordList ToKeywords(const std::string& value) {
  const std::string lowercase_value = base::ToLowerASCII(value);

  const std::string stripped_value =
      StripNonAlphaNumericCharacters(lowercase_value);

  return base::SplitString(stripped_value, " ", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

bool IsSubset(KeywordList keywords_lhs, KeywordList keywords_rhs) {
  base::ranges::sort(keywords_lhs);
  base::ranges::sort(keywords_rhs);
  return base::ranges::includes(keywords_lhs, keywords_rhs);
}

}  // namespace

PurchaseIntent::PurchaseIntent(resource::PurchaseIntent& resource)
    : resource_(resource) {
  AdsClientHelper::AddObserver(this);
  TabManager::GetInstance().AddObserver(this);
}

PurchaseIntent::~PurchaseIntent() {
  AdsClientHelper::RemoveObserver(this);
  TabManager::GetInstance().RemoveObserver(this);
}

void PurchaseIntent::Process(const GURL& url) {
  if (!resource_->IsInitialized()) {
    BLOG(1,
         "Failed to process purchase intent signal for visited URL due to "
         "uninitialized purchase intent resource");

    return;
  }

  if (!url.is_valid()) {
    BLOG(1,
         "Failed to process purchase intent signal for visited URL due to "
         "an invalid url");

    return;
  }

  const targeting::PurchaseIntentSignalInfo purchase_intent_signal =
      ExtractSignal(url);

  if (purchase_intent_signal.segments.empty()) {
    BLOG(1, "No purchase intent matches found for visited URL");
    return;
  }

  BLOG(1, "Extracted purchase intent signal from visited URL");

  AppendIntentSignalToHistory(purchase_intent_signal);
}

///////////////////////////////////////////////////////////////////////////////

targeting::PurchaseIntentSignalInfo PurchaseIntent::ExtractSignal(
    const GURL& url) const {
  targeting::PurchaseIntentSignalInfo purchase_intent_signal;

  const absl::optional<std::string> search_query =
      ExtractSearchTermQueryValue(url);
  if (search_query) {
    const SegmentList keyword_segments =
        GetSegmentsForSearchQuery(*search_query);
    if (!keyword_segments.empty()) {
      purchase_intent_signal.created_at = base::Time::Now();
      purchase_intent_signal.segments = keyword_segments;
      purchase_intent_signal.weight =
          GetFunnelWeightForSearchQuery(*search_query);
    }
  } else {
    const targeting::PurchaseIntentSiteInfo purchase_intent_site = GetSite(url);

    if (purchase_intent_site.url_netloc.is_valid()) {
      purchase_intent_signal.created_at = base::Time::Now();
      purchase_intent_signal.segments = purchase_intent_site.segments;
      purchase_intent_signal.weight = purchase_intent_site.weight;
    }
  }

  return purchase_intent_signal;
}

targeting::PurchaseIntentSiteInfo PurchaseIntent::GetSite(
    const GURL& url) const {
  targeting::PurchaseIntentSiteInfo purchase_intent_site;

  const targeting::PurchaseIntentInfo* const purchase_intent = resource_->Get();
  DCHECK(purchase_intent);

  for (const auto& site : purchase_intent->sites) {
    if (SameDomainOrHost(url, site.url_netloc)) {
      purchase_intent_site = site;
      break;
    }
  }

  return purchase_intent_site;
}

SegmentList PurchaseIntent::GetSegmentsForSearchQuery(
    const std::string& search_query) const {
  SegmentList segments;

  const KeywordList search_query_keywords = ToKeywords(search_query);

  const targeting::PurchaseIntentInfo* const purchase_intent = resource_->Get();
  DCHECK(purchase_intent);

  for (const auto& keyword : purchase_intent->segment_keywords) {
    // Intended behavior relies on early return from list traversal and
    // implicitely on the ordering of |segment_keywords_| to ensure specific
    // segments are matched over general segments, e.g. "audi a6" segments
    // should be returned over "audi" segments if possible
    if (IsSubset(search_query_keywords, ToKeywords(keyword.keywords))) {
      segments = keyword.segments;
      break;
    }
  }

  return segments;
}

uint16_t PurchaseIntent::GetFunnelWeightForSearchQuery(
    const std::string& search_query) const {
  const KeywordList search_query_keywords = ToKeywords(search_query);

  uint16_t max_weight = kPurchaseIntentDefaultSignalWeight;

  const targeting::PurchaseIntentInfo* const purchase_intent = resource_->Get();
  DCHECK(purchase_intent);

  for (const auto& keyword : purchase_intent->funnel_keywords) {
    const KeywordList keywords = ToKeywords(keyword.keywords);

    if (IsSubset(search_query_keywords, keywords) &&
        keyword.weight > max_weight) {
      max_weight = keyword.weight;
    }
  }

  return max_weight;
}

void PurchaseIntent::OnNotifyLocaleDidChange(const std::string& /*locale*/) {
  resource_->Load();
}

void PurchaseIntent::OnNotifyDidUpdateResourceComponent(const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    resource_->Load();
  }
}

void PurchaseIntent::OnTextContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& /*content*/) {
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

}  // namespace brave_ads::processor
