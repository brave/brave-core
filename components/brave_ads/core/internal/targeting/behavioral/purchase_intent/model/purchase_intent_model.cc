/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include "base/containers/adapters.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads {

namespace {

constexpr uint16_t kSignalLevel = 1;
constexpr size_t kMaximumSegments = 3;

uint16_t CalculateScoreForHistory(
    const PurchaseIntentSignalHistoryList& history) {
  uint16_t score = 0;

  const base::TimeDelta time_window = kPurchaseIntentTimeWindow.Get();

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

SegmentList PurchaseIntentModel::GetSegments() const {
  SegmentList segments;

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  if (purchase_intent_signal_history.empty()) {
    return segments;
  }

  std::multimap<uint16_t, std::string> scores;
  for (const auto& [segment, history] : purchase_intent_signal_history) {
    const uint16_t score = CalculateScoreForHistory(history);
    scores.insert(std::make_pair(score, segment));
  }

  const uint16_t threshold = kPurchaseIntentThreshold.Get();

  for (const auto& [score, segment] : base::Reversed(scores)) {
    if (score >= threshold) {
      segments.push_back(segment);
      if (segments.size() >= kMaximumSegments) {
        break;
      }
    }
  }

  return segments;
}

}  // namespace brave_ads
