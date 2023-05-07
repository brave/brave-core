/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace brave_ads {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const base::Time created_at,
    const int weight)
    : created_at(created_at), weight(weight) {}

// TODO(https://github.com/brave/brave-browser/issues/27893):
bool operator==(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  return lhs.created_at == rhs.created_at && lhs.weight == rhs.weight;
}

bool operator!=(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
