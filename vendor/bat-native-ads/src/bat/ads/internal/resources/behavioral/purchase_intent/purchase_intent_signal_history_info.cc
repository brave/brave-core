/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads::targeting {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(
    const base::Time created_at,
    const uint16_t weight)
    : created_at(created_at), weight(weight) {}

bool operator==(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  return DoubleEquals(lhs.created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
         lhs.weight == rhs.weight;
}

bool operator!=(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads::targeting
