/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_

#include <cstdint>
#include <string>

#include "bat/ads/internal/segments/segments_alias.h"

namespace ads {
namespace ad_targeting {

struct PurchaseIntentSiteInfo {
 public:
  PurchaseIntentSiteInfo();
  PurchaseIntentSiteInfo(const SegmentList& segments,
                         const std::string& url_netloc,
                         const uint16_t weight);
  PurchaseIntentSiteInfo(const PurchaseIntentSiteInfo& info);
  ~PurchaseIntentSiteInfo();

  bool operator==(const PurchaseIntentSiteInfo& rhs) const;
  bool operator!=(const PurchaseIntentSiteInfo& rhs) const;

  SegmentList segments;
  std::string url_netloc;
  uint16_t weight = 0;
};

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_
