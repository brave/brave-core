/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT

#include <stdint.h>

#include <vector>

#include "bat/ads/internal/ad_targeting/resources/purchase_intent/site_info.h"
#include "bat/ads/internal/ad_targeting/resources/purchase_intent/segment_keyword_info.h"
#include "bat/ads/internal/ad_targeting/resources/purchase_intent/funnel_keyword_info.h"

namespace ads {
namespace ad_targeting {
namespace resource {

struct PurchaseIntentInfo {
 public:
  PurchaseIntentInfo();
  PurchaseIntentInfo(
      const PurchaseIntentInfo& info);
  ~PurchaseIntentInfo();

  uint16_t version = 0;
  std::vector<SiteInfo> sites;
  std::vector<SegmentKeywordInfo> segment_keywords;
  std::vector<FunnelKeywordInfo> funnel_keywords;
};

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_  // NOLINT
