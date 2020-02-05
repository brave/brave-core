/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <functional>
#include <utility>
#include <algorithm>

#include "bat/ads/internal/purchase_intent/purchase_intent_classifier.h"
#include "bat/ads/internal/purchase_intent/funnel_sites.h"
#include "bat/ads/internal/purchase_intent/keywords.h"
#include "bat/ads/internal/time.h"

namespace ads {

PurchaseIntentClassifier::PurchaseIntentClassifier(
    const uint16_t signal_level,
    const uint16_t classification_threshold,
    const uint64_t signal_decay_time_window_in_seconds)
    : signal_level_(signal_level),
      classification_threshold_(classification_threshold),
      signal_decay_time_window_in_seconds_(
          signal_decay_time_window_in_seconds) {
}

PurchaseIntentClassifier::~PurchaseIntentClassifier() = default;

PurchaseIntentSignalInfo PurchaseIntentClassifier::ExtractIntentSignal(
    const std::string& url) {
  PurchaseIntentSignalInfo signal_info;
  const std::string search_query =
      SearchProviders::ExtractSearchQueryKeywords(url);

  if (!search_query.empty()) {
    auto keyword_segments = Keywords::GetSegments(search_query);

    if (!keyword_segments.empty()) {
      uint16_t keyword_weight = Keywords::GetFunnelWeight(search_query);

      signal_info.timestamp_in_seconds = Time::NowInSeconds();
      signal_info.segments = keyword_segments;
      signal_info.weight = keyword_weight;
      return signal_info;
    }
  } else {
    FunnelSiteInfo info = FunnelSites::GetFunnelSite(url);

    if (!info.url_netloc.empty()) {
      signal_info.timestamp_in_seconds = Time::NowInSeconds();
      signal_info.segments = info.segments;
      signal_info.weight = info.weight;
      return signal_info;
    }
  }

  return signal_info;
}

PurchaseIntentWinningCategoryList
PurchaseIntentClassifier::GetWinningCategories(
    const PurchaseIntentSignalSegmentHistoryMap& history,
    uint16_t max_segments) {
  PurchaseIntentWinningCategoryList winning_categories;
  if (history.empty()) {
    return winning_categories;
  }

  std::multimap<uint16_t, std::string> scores;
  for (const auto& segment_history : history) {
    uint16_t score = GetIntentScoreForHistory(segment_history.second);
    scores.insert(std::make_pair(score, segment_history.first));
  }

  std::multimap<uint16_t, std::string>::reverse_iterator rit;
  for (rit=scores.rbegin(); rit != scores.rend(); ++rit) {
    if (rit->first > classification_threshold_) {
      winning_categories.push_back(rit->second);
    }

    if (winning_categories.size() >= max_segments) {
      return winning_categories;
    }
  }

  return winning_categories;
}

uint16_t PurchaseIntentClassifier::GetIntentScoreForHistory(
    const PurchaseIntentSignalSegmentHistoryList& history) {
  uint16_t intent_score = 0;

  for (const auto& signal_segment : history) {
    const base::Time signal_decayed_at_in_seconds =
        Time::FromDoubleT(signal_segment.timestamp_in_seconds) +
        base::TimeDelta::FromSeconds(signal_decay_time_window_in_seconds_);
    const base::Time now_in_seconds = base::Time::Now();

    if (now_in_seconds > signal_decayed_at_in_seconds) {
      continue;
    }

    intent_score += signal_level_ * signal_segment.weight;
  }

  return intent_score;
}

}  // namespace ads
