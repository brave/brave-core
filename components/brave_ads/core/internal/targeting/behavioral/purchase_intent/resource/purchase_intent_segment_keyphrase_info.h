/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYPHRASE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYPHRASE_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/keyphrase/purchase_intent_keyphrase_alias.h"

namespace brave_ads {

struct PurchaseIntentSegmentKeyphraseInfo final {
  PurchaseIntentSegmentKeyphraseInfo();
  PurchaseIntentSegmentKeyphraseInfo(SegmentList segments,
                                     KeywordList keywords);

  PurchaseIntentSegmentKeyphraseInfo(
      const PurchaseIntentSegmentKeyphraseInfo&) = delete;
  PurchaseIntentSegmentKeyphraseInfo& operator=(
      const PurchaseIntentSegmentKeyphraseInfo&) = delete;

  PurchaseIntentSegmentKeyphraseInfo(
      PurchaseIntentSegmentKeyphraseInfo&&) noexcept;
  PurchaseIntentSegmentKeyphraseInfo& operator=(
      PurchaseIntentSegmentKeyphraseInfo&&) noexcept;

  ~PurchaseIntentSegmentKeyphraseInfo();

  SegmentList segments;
  KeywordList keywords;
};

using PurchaseIntentSegmentKeyphraseList =
    std::vector<PurchaseIntentSegmentKeyphraseInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_SEGMENT_KEYPHRASE_INFO_H_
