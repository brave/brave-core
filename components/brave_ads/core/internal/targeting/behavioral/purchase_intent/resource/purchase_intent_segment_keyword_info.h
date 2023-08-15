/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct PurchaseIntentSegmentKeywordInfo final {
  PurchaseIntentSegmentKeywordInfo();
  PurchaseIntentSegmentKeywordInfo(SegmentList segments, std::string keywords);

  PurchaseIntentSegmentKeywordInfo(const PurchaseIntentSegmentKeywordInfo&);
  PurchaseIntentSegmentKeywordInfo& operator=(
      const PurchaseIntentSegmentKeywordInfo&);

  PurchaseIntentSegmentKeywordInfo(PurchaseIntentSegmentKeywordInfo&&) noexcept;
  PurchaseIntentSegmentKeywordInfo& operator=(
      PurchaseIntentSegmentKeywordInfo&&) noexcept;

  ~PurchaseIntentSegmentKeywordInfo();

  SegmentList segments;
  std::string keywords;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_
