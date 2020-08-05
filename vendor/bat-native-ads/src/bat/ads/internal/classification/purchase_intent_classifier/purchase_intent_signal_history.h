/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_SIGNAL_HISTORY_H_  // NOLINT
#define BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_SIGNAL_HISTORY_H_  // NOLINT

#include <stdint.h>

#include <deque>
#include <map>
#include <string>

#include "bat/ads/result.h"

namespace ads {

struct PurchaseIntentSignalHistory {
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
      const std::string& json);

  uint64_t timestamp_in_seconds;
  uint16_t weight = 0;
};

using PurchaseIntentSignalSegmentHistoryList =
    std::deque<PurchaseIntentSignalHistory>;
using PurchaseIntentSignalSegmentHistoryMap =
    std::map<std::string, PurchaseIntentSignalSegmentHistoryList>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_SIGNAL_HISTORY_H_  // NOLINT
