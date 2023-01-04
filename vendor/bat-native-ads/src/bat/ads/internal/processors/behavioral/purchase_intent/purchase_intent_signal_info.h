/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_

#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::targeting {

struct PurchaseIntentSignalInfo final {
  PurchaseIntentSignalInfo();

  PurchaseIntentSignalInfo(const PurchaseIntentSignalInfo& other);
  PurchaseIntentSignalInfo& operator=(const PurchaseIntentSignalInfo& other);

  PurchaseIntentSignalInfo(PurchaseIntentSignalInfo&& other) noexcept;
  PurchaseIntentSignalInfo& operator=(
      PurchaseIntentSignalInfo&& other) noexcept;

  ~PurchaseIntentSignalInfo();

  base::Time created_at;
  SegmentList segments;
  uint16_t weight = 0;
};

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
