/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

#include <tuple>

namespace brave_ads {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const base::Time created_at,
    const int weight)
    : created_at(created_at), weight(weight) {}

bool operator==(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  const auto tie = [](const PurchaseIntentSignalHistoryInfo&
                          purchase_intent_signal_history) {
    return std::tie(purchase_intent_signal_history.created_at,
                    purchase_intent_signal_history.weight);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
