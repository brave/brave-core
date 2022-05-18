/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_

#include <cstdint>

#include "bat/ads/internal/segments/segments_aliases.h"
#include "url/gurl.h"

namespace ads {
namespace targeting {

struct PurchaseIntentSiteInfo final {
 public:
  PurchaseIntentSiteInfo();
  PurchaseIntentSiteInfo(const SegmentList& segments,
                         const GURL& url_netloc,
                         const uint16_t weight);
  PurchaseIntentSiteInfo(const PurchaseIntentSiteInfo& info);
  ~PurchaseIntentSiteInfo();

  bool operator==(const PurchaseIntentSiteInfo& rhs) const;
  bool operator!=(const PurchaseIntentSiteInfo& rhs) const;

  SegmentList segments;
  GURL url_netloc;
  uint16_t weight = 0;
};

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SITE_INFO_H_
