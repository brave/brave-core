/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_  // NOLINT

#include <stdint.h>

#include <string>

#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_segment_keyword_info.h"

namespace ads {

struct PurchaseIntentSiteInfo {
 public:
  PurchaseIntentSiteInfo();
  PurchaseIntentSiteInfo(
      const SegmentList& segments,
      const std::string& url_netloc,
      const uint16_t weight);
  PurchaseIntentSiteInfo(
      const PurchaseIntentSiteInfo& info);
  ~PurchaseIntentSiteInfo();

  bool operator==(
      const PurchaseIntentSiteInfo& rhs) const;
  bool operator!=(
      const PurchaseIntentSiteInfo& rhs) const;

  SegmentList segments;
  std::string url_netloc;
  uint16_t weight = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_  // NOLINT
