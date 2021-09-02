/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_

#include <cstdint>
#include <string>

namespace ads {
namespace ad_targeting {

struct PurchaseIntentSignalHistoryInfo {
  PurchaseIntentSignalHistoryInfo();
  PurchaseIntentSignalHistoryInfo(const int64_t timestamp_in_seconds,
                                  const uint16_t weight);
  PurchaseIntentSignalHistoryInfo(const PurchaseIntentSignalHistoryInfo& info);
  ~PurchaseIntentSignalHistoryInfo();

  bool operator==(const PurchaseIntentSignalHistoryInfo& rhs) const;
  bool operator!=(const PurchaseIntentSignalHistoryInfo& rhs) const;

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  int64_t timestamp_in_seconds = 0;
  uint16_t weight = 0;
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_
