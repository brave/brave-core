/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_  // NOLINT

#include <deque>
#include <map>
#include <string>

#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads {

using PurchaseIntentSignalHistoryList =
    std::deque<PurchaseIntentSignalHistoryInfo>;
using PurchaseIntentSignalHistoryMap =
    std::map<std::string, PurchaseIntentSignalHistoryList>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_  // NOLINT
