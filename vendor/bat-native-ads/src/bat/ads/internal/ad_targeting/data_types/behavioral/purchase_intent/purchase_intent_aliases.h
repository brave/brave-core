/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_

#include <deque>
#include <map>
#include <string>

#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads {
namespace ad_targeting {

using PurchaseIntentSignalHistoryList =
    std::deque<PurchaseIntentSignalHistoryInfo>;
using PurchaseIntentSignalHistoryMap =
    std::map<std::string, PurchaseIntentSignalHistoryList>;

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_ALIASES_H_
