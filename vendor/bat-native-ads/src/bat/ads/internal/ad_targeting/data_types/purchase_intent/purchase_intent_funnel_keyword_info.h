/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_  // NOLINT

#include <stdint.h>

#include <string>

namespace ads {

struct PurchaseIntentFunnelKeywordInfo {
 public:
  PurchaseIntentFunnelKeywordInfo();
  PurchaseIntentFunnelKeywordInfo(
      const std::string& keywords,
      const uint16_t weight);
  ~PurchaseIntentFunnelKeywordInfo();

  std::string keywords;
  uint16_t weight = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_  // NOLINT
