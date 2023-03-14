/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "base/time/time.h"

namespace ads::targeting {

struct PurchaseIntentSignalHistoryInfo final {
  PurchaseIntentSignalHistoryInfo(base::Time created_at, uint16_t weight);

  base::Time created_at;
  uint16_t weight = 0;
};

bool operator==(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs);
bool operator!=(const PurchaseIntentSignalHistoryInfo& lhs,
                const PurchaseIntentSignalHistoryInfo& rhs);

using PurchaseIntentSignalHistoryList =
    std::vector<PurchaseIntentSignalHistoryInfo>;
using PurchaseIntentSignalHistoryMap =
    std::map<std::string, PurchaseIntentSignalHistoryList>;

}  // namespace ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_
