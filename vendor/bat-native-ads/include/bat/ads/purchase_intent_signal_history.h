/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_PURCHASE_INTENT_SIGNAL_HISTORY_H_
#define BAT_ADS_PURCHASE_INTENT_SIGNAL_HISTORY_H_

#include <stdint.h>
#include <string>
#include <map>
#include <deque>

#include "bat/ads/export.h"
#include "bat/ads/result.h"

namespace ads {

struct ADS_EXPORT PurchaseIntentSignalHistory {
  PurchaseIntentSignalHistory();
  PurchaseIntentSignalHistory(
      const PurchaseIntentSignalHistory& properties);
  ~PurchaseIntentSignalHistory();

  bool operator==(
      const PurchaseIntentSignalHistory& rhs) const;

  bool operator!=(
      const PurchaseIntentSignalHistory& rhs) const;

  std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  uint64_t timestamp_in_seconds;
  uint16_t weight = 0;
};

using PurchaseIntentSignalSegmentHistoryList =
    std::deque<PurchaseIntentSignalHistory>;
using PurchaseIntentSignalSegmentHistoryMap =
    std::map<std::string, PurchaseIntentSignalSegmentHistoryList>;

}  // namespace ads

#endif  // BAT_ADS_PURCHASE_INTENT_SIGNAL_HISTORY_H_
