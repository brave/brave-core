/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_funnel_keyword_info.h"
#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_segment_keyword_info.h"
#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_site_info.h"

namespace base {
class Value;
}  // namespace base

namespace ads {
namespace targeting {

struct PurchaseIntentInfo final {
 public:
  PurchaseIntentInfo();
  ~PurchaseIntentInfo();

  PurchaseIntentInfo(const PurchaseIntentInfo& info) = delete;
  PurchaseIntentInfo& operator=(const PurchaseIntentInfo& info) = delete;

  static std::unique_ptr<PurchaseIntentInfo> CreateFromValue(
      base::Value resource_value,
      std::string* error_message);

  uint16_t version = 0;
  std::vector<PurchaseIntentSiteInfo> sites;
  std::vector<PurchaseIntentSegmentKeywordInfo> segment_keywords;
  std::vector<PurchaseIntentFunnelKeywordInfo> funnel_keywords;
};

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_INFO_H_
