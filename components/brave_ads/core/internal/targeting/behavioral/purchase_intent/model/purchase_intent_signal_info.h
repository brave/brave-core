/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_SIGNAL_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_SIGNAL_INFO_H_

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct PurchaseIntentSignalInfo final {
  PurchaseIntentSignalInfo();

  PurchaseIntentSignalInfo(base::Time at, SegmentList segments, int weight);

  PurchaseIntentSignalInfo(const PurchaseIntentSignalInfo&);
  PurchaseIntentSignalInfo& operator=(const PurchaseIntentSignalInfo&);

  PurchaseIntentSignalInfo(PurchaseIntentSignalInfo&&) noexcept;
  PurchaseIntentSignalInfo& operator=(PurchaseIntentSignalInfo&&) noexcept;

  ~PurchaseIntentSignalInfo();

  base::Time at;
  SegmentList segments;
  int weight = 0;
};

using PurchaseIntentSignalList = std::vector<PurchaseIntentSignalInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_SIGNAL_INFO_H_
