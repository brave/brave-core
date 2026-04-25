/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_predictor.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_scoring.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads {

void BuyPurchaseIntentSignal(
    const PurchaseIntentSignalInfo& purchase_intent_signal) {
  const PurchaseIntentSignalHistoryInfo signal_history(
      purchase_intent_signal.at, purchase_intent_signal.weight);

  for (const auto& segment : purchase_intent_signal.segments) {
    ClientStateManager::GetInstance()
        .AppendToPurchaseIntentSignalHistoryForSegment(segment, signal_history);
  }
}

SegmentList GetPurchaseIntentSegments() {
  const PurchaseIntentSignalHistoryMap& signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const std::multimap</*score*/ int, /*segment*/ std::string> segment_scores =
      ComputePurchaseIntentSignalHistorySegmentScores(signal_history);

  return PredictPurchaseIntentSegments(segment_scores);
}

}  // namespace brave_ads
