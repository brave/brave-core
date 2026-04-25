/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_FUNNEL_KEYPHRASE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_FUNNEL_KEYPHRASE_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_alias.h"

namespace brave_ads {

struct PurchaseIntentFunnelKeyphraseInfo final {
  PurchaseIntentFunnelKeyphraseInfo();

  PurchaseIntentFunnelKeyphraseInfo(const PurchaseIntentFunnelKeyphraseInfo&) =
      delete;
  PurchaseIntentFunnelKeyphraseInfo& operator=(
      const PurchaseIntentFunnelKeyphraseInfo&) = delete;

  PurchaseIntentFunnelKeyphraseInfo(
      PurchaseIntentFunnelKeyphraseInfo&& other) noexcept;
  PurchaseIntentFunnelKeyphraseInfo& operator=(
      PurchaseIntentFunnelKeyphraseInfo&& other) noexcept;

  ~PurchaseIntentFunnelKeyphraseInfo();

  KeywordList keywords;
  int weight = 0;
};

using PurchaseIntentFunnelKeyphraseList =
    std::vector<PurchaseIntentFunnelKeyphraseInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_FUNNEL_KEYPHRASE_INFO_H_
