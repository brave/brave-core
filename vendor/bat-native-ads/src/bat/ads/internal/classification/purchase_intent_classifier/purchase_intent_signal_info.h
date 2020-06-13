/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
#define BAT_ADS_INTERNAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "bat/ads/internal/classification/purchase_intent_classifier/segment_keyword_info.h"

namespace ads {

struct PurchaseIntentSignalInfo {
 public:
  PurchaseIntentSignalInfo();
  PurchaseIntentSignalInfo(
      const PurchaseIntentSignalInfo& info);
  ~PurchaseIntentSignalInfo();

  uint64_t timestamp_in_seconds = 0;
  PurchaseIntentSegmentList segments;
  uint16_t weight = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
