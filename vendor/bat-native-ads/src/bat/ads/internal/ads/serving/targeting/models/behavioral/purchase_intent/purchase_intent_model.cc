/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/models/behavioral/purchase_intent/purchase_intent_model.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/features/purchase_intent_features.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads {
namespace targeting {
namespace model {

namespace {

constexpr uint16_t kSignalLevel = 1;
constexpr size_t kMaximumSegments = 3;

uint16_t CalculateScoreForHistory(
    const PurchaseIntentSignalHistoryList& history) {
  uint16_t score = 0;

  const base::TimeDelta time_window = features::GetPurchaseIntentTimeWindow();
  for (const auto& signal_segment : history) {
    const base::Time signal_decayed_time =
        signal_segment.created_at + time_window;

    if (base::Time::Now() > signal_decayed_time) {
      continue;
    }

    score += kSignalLevel * signal_segment.weight;
  }

  return score;
}

}  // namespace

PurchaseIntent::PurchaseIntent() = default;

PurchaseIntent::~PurchaseIntent() = default;

SegmentList PurchaseIntent::GetSegments() const {
  SegmentList segments;

  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance()->GetPurchaseIntentSignalHistory();

  if (history.empty()) {
    return segments;
  }

  std::multimap<uint16_t, std::string> scores;
  for (const auto& segment_history : history) {
    const uint16_t score = CalculateScoreForHistory(segment_history.second);
    scores.insert(std::make_pair(score, segment_history.first));
  }

  const uint16_t threshold = features::GetPurchaseIntentThreshold();
  std::multimap<uint16_t, std::string>::reverse_iterator iter;
  for (iter = scores.rbegin(); iter != scores.rend(); ++iter) {
    if (iter->first >= threshold) {
      segments.push_back(iter->second);
    }

    if (segments.size() >= kMaximumSegments) {
      break;
    }
  }

  return segments;
}

}  // namespace model
}  // namespace targeting
}  // namespace ads
