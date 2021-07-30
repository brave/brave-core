/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"

#include <algorithm>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_signal_history_info.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/purchase_intent/purchase_intent_processor_values.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/search_engine/search_providers.h"
#include "bat/ads/internal/string_util.h"
#include "bat/ads/internal/url_util.h"

namespace ads {
namespace ad_targeting {
namespace processor {

using KeywordList = std::vector<std::string>;

namespace {

void AppendIntentSignalToHistory(
    const PurchaseIntentSignalInfo& purchase_intent_signal) {
  for (const auto& segment : purchase_intent_signal.segments) {
    PurchaseIntentSignalHistoryInfo history;
    history.timestamp_in_seconds = purchase_intent_signal.timestamp_in_seconds;
    history.weight = purchase_intent_signal.weight;

    Client::Get()->AppendToPurchaseIntentSignalHistoryForSegment(segment,
                                                                 history);
  }
}

KeywordList ToKeywords(const std::string& value) {
  const std::string lowercase_value = base::ToLowerASCII(value);

  const std::string stripped_value =
      StripNonAlphaNumericCharacters(lowercase_value);

  const KeywordList keywords = base::SplitString(
      stripped_value, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  return keywords;
}

bool IsSubset(const KeywordList& keywords_lhs,
              const KeywordList& keywords_rhs) {
  KeywordList sorted_keywords_lhs = keywords_lhs;
  std::sort(sorted_keywords_lhs.begin(), sorted_keywords_lhs.end());

  KeywordList sorted_keywords_rhs = keywords_rhs;
  std::sort(sorted_keywords_rhs.begin(), sorted_keywords_rhs.end());

  return std::includes(sorted_keywords_lhs.begin(), sorted_keywords_lhs.end(),
                       sorted_keywords_rhs.begin(), sorted_keywords_rhs.end());
}

}  // namespace

PurchaseIntent::PurchaseIntent(resource::PurchaseIntent* resource)
    : resource_(resource) {
  DCHECK(resource_);
}

PurchaseIntent::~PurchaseIntent() = default;

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

  const PurchaseIntentSignalInfo purchase_intent_signal = ExtractSignal(url);

  if (purchase_intent_signal.segments.empty()) {
    BLOG(1, "No purchase intent matches found for visited URL");
    return;
  }

  BLOG(1, "Extracted purchase intent signal from visited URL");

  AppendIntentSignalToHistory(purchase_intent_signal);
}

///////////////////////////////////////////////////////////////////////////////

PurchaseIntentSignalInfo PurchaseIntent::ExtractSignal(const GURL& url) const {
  PurchaseIntentSignalInfo signal_info;

  const std::string search_query =
      SearchProviders::ExtractSearchQueryKeywords(url.spec());

  if (!search_query.empty()) {
    const SegmentList keyword_segments =
        GetSegmentsForSearchQuery(search_query);

    if (!keyword_segments.empty()) {
      const uint16_t keyword_weight =
          GetFunnelWeightForSearchQuery(search_query);

      signal_info.timestamp_in_seconds =
          static_cast<uint64_t>(base::Time::Now().ToDoubleT());
      signal_info.segments = keyword_segments;
      signal_info.weight = keyword_weight;
    }
  } else {
    PurchaseIntentSiteInfo info = GetSite(url);

    if (!info.url_netloc.empty()) {
      signal_info.timestamp_in_seconds =
          static_cast<uint64_t>(base::Time::Now().ToDoubleT());
      signal_info.segments = info.segments;
      signal_info.weight = info.weight;
    }
  }

  return signal_info;
}

PurchaseIntentSiteInfo PurchaseIntent::GetSite(const GURL& url) const {
  PurchaseIntentSiteInfo info;

  const PurchaseIntentInfo purchase_intent = resource_->get();

  for (const auto& site : purchase_intent.sites) {
    if (SameDomainOrHost(url.spec(), site.url_netloc)) {
      info = site;
      break;
    }
  }

  return info;
}

SegmentList PurchaseIntent::GetSegmentsForSearchQuery(
    const std::string& search_query) const {
  SegmentList segments;

  const KeywordList search_query_keywords = ToKeywords(search_query);

  const PurchaseIntentInfo purchase_intent = resource_->get();

  for (const auto& keyword : purchase_intent.segment_keywords) {
    const KeywordList keywords = ToKeywords(keyword.keywords);

    // Intended behavior relies on early return from list traversal and
    // implicitely on the ordering of |segment_keywords_| to ensure specific
    // segments are matched over general segments, e.g. "audi a6" segments
    // should be returned over "audi" segments if possible
    if (IsSubset(search_query_keywords, keywords)) {
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

  const PurchaseIntentInfo purchase_intent = resource_->get();

  for (const auto& keyword : purchase_intent.funnel_keywords) {
    const KeywordList keywords = ToKeywords(keyword.keywords);

    if (IsSubset(search_query_keywords, keywords) &&
        keyword.weight > max_weight) {
      max_weight = keyword.weight;
    }
  }

  return max_weight;
}

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads
