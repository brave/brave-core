/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_INFO_H_

#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_keyphrase_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_segment_keyphrase_info.h"

namespace brave_ads {

struct PurchaseIntentInfo final {
  PurchaseIntentInfo();

  PurchaseIntentInfo(const PurchaseIntentInfo&) = delete;
  PurchaseIntentInfo& operator=(const PurchaseIntentInfo&) = delete;

  PurchaseIntentInfo(PurchaseIntentInfo&&) noexcept;
  PurchaseIntentInfo& operator=(PurchaseIntentInfo&&) noexcept;

  ~PurchaseIntentInfo();

  static base::expected<PurchaseIntentInfo, std::string> CreateFromValue(
      base::Value::Dict dict);

  int version = 0;
  PurchaseIntentSegmentKeyphraseList segment_keyphrases;
  PurchaseIntentFunnelKeyphraseList funnel_keyphrases;
  PurchaseIntentFunnelSiteMap funnel_sites;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_INFO_H_
