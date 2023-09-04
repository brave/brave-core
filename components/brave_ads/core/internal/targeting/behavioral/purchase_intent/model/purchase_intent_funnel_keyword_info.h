/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_

#include <cstdint>
#include <string>

namespace brave_ads {

struct PurchaseIntentFunnelKeywordInfo final {
  PurchaseIntentFunnelKeywordInfo();
  PurchaseIntentFunnelKeywordInfo(std::string keywords, uint16_t weight);

  std::string keywords;
  uint16_t weight = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_MODEL_PURCHASE_INTENT_FUNNEL_KEYWORD_INFO_H_
