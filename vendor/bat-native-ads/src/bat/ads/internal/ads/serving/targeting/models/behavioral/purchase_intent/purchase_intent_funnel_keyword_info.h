/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_

#include <cstdint>
#include <string>

namespace ads::targeting {

struct PurchaseIntentFunnelKeywordInfo final {
  PurchaseIntentFunnelKeywordInfo();
  PurchaseIntentFunnelKeywordInfo(const std::string& keywords, uint16_t weight);

  PurchaseIntentFunnelKeywordInfo(const PurchaseIntentFunnelKeywordInfo& other);
  PurchaseIntentFunnelKeywordInfo& operator=(
      const PurchaseIntentFunnelKeywordInfo& other);

  PurchaseIntentFunnelKeywordInfo(
      PurchaseIntentFunnelKeywordInfo&& other) noexcept;
  PurchaseIntentFunnelKeywordInfo& operator=(
      PurchaseIntentFunnelKeywordInfo&& other) noexcept;

  ~PurchaseIntentFunnelKeywordInfo();

  std::string keywords;
  uint16_t weight = 0;
};

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_MODELS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_
