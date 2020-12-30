/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT

#include <stdint.h>

#include <vector>

#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_funnel_keyword_info.h"
#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_segment_keyword_info.h"
#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_site_info.h"

namespace ads {

struct PurchaseIntentInfo {
 public:
  PurchaseIntentInfo();
  PurchaseIntentInfo(
      const PurchaseIntentInfo& info);
  ~PurchaseIntentInfo();

  uint16_t version = 0;
  std::vector<PurchaseIntentSiteInfo> sites;
  std::vector<PurchaseIntentSegmentKeywordInfo> segment_keywords;
  std::vector<PurchaseIntentFunnelKeywordInfo> funnel_keywords;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT
