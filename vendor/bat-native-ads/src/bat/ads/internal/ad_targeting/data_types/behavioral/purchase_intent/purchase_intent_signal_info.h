/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_

#include <cstdint>

#include "bat/ads/internal/segments/segments_alias.h"

namespace ads {
namespace ad_targeting {

struct PurchaseIntentSignalInfo {
 public:
  PurchaseIntentSignalInfo();
  PurchaseIntentSignalInfo(const PurchaseIntentSignalInfo& info);
  ~PurchaseIntentSignalInfo();

  uint64_t timestamp_in_seconds = 0;
  SegmentList segments;
  uint16_t weight = 0;
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
