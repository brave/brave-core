/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/purchase_intent/purchase_intent_model.h"

#include <map>
#include <string>

#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {
const uint16_t kSignalLevel = 1;
const uint16_t kThreshold = 3;
const int64_t kTimeWindowInSeconds = 7 * (24 * base::Time::kSecondsPerHour);
const size_t kMaximumSegments = 3;
}  // namespace

PurchaseIntent::PurchaseIntent() = default;

PurchaseIntent::~PurchaseIntent() = default;

SegmentList PurchaseIntent::GetSegments() const {
  const PurchaseIntentSignalSegmentHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  if (history.empty()) {
    return {};
  }

  std::multimap<uint16_t, std::string> scores;
  for (const auto& segment_history : history) {
    uint16_t score = GetIntentScoreForHistory(segment_history.second);
    scores.insert(std::make_pair(score, segment_history.first));
  }

  SegmentList segments;
  std::multimap<uint16_t, std::string>::reverse_iterator rit;
  for (rit=scores.rbegin(); rit != scores.rend(); ++rit) {
    if (rit->first >= kThreshold) {
      segments.push_back(rit->second);
    }

    if (segments.size() >= kMaximumSegments) {
      return segments;
    }
  }

  return segments;
}

uint16_t PurchaseIntent::GetIntentScoreForHistory(
    const PurchaseIntentSignalSegmentHistoryList& history) const {
  uint16_t intent_score = 0;

  for (const auto& signal_segment : history) {
    const base::Time signal_decayed_at_in_seconds =
        base::Time::FromDoubleT(signal_segment.timestamp_in_seconds) +
            base::TimeDelta::FromSeconds(kTimeWindowInSeconds);

    const base::Time now_in_seconds = base::Time::Now();

    if (now_in_seconds > signal_decayed_at_in_seconds) {
      continue;
    }

    intent_score += kSignalLevel * signal_segment.weight;
  }

  return intent_score;
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
