/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_

#include <cstdint>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads::targeting {

struct PurchaseIntentSignalInfo final {
  PurchaseIntentSignalInfo();

  PurchaseIntentSignalInfo(const PurchaseIntentSignalInfo&);
  PurchaseIntentSignalInfo& operator=(const PurchaseIntentSignalInfo&);

  PurchaseIntentSignalInfo(PurchaseIntentSignalInfo&&) noexcept;
  PurchaseIntentSignalInfo& operator=(PurchaseIntentSignalInfo&&) noexcept;

  ~PurchaseIntentSignalInfo();

  base::Time created_at;
  SegmentList segments;
  uint16_t weight = 0;
};

}  // namespace brave_ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_INFO_H_
