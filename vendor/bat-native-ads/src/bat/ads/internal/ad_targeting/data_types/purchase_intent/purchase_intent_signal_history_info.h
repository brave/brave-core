/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_  // NOLINT

#include <stdint.h>

#include <string>

#include "bat/ads/result.h"

namespace ads {

struct PurchaseIntentSignalHistoryInfo {
  PurchaseIntentSignalHistoryInfo();
  PurchaseIntentSignalHistoryInfo(
      const int64_t timestamp_in_seconds,
      const uint16_t weight);
  PurchaseIntentSignalHistoryInfo(
      const PurchaseIntentSignalHistoryInfo& info);
  ~PurchaseIntentSignalHistoryInfo();

  bool operator==(
      const PurchaseIntentSignalHistoryInfo& rhs) const;
  bool operator!=(
      const PurchaseIntentSignalHistoryInfo& rhs) const;

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  int64_t timestamp_in_seconds = 0;
  uint16_t weight = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_  // NOLINT
