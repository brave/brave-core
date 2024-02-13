/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_VALUE_UTIL_H_

#include <optional>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_keyphrase_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_segment_keyphrase_info.h"

namespace brave_ads {

std::optional<int> ParseVersion(const base::Value::Dict& dict);

std::optional<SegmentList> ParseSegments(const base::Value::Dict& dict);

std::optional<PurchaseIntentSegmentKeyphraseList> ParseSegmentKeyphrases(
    const SegmentList& segments,
    const base::Value::Dict& dict);

std::optional<PurchaseIntentFunnelKeyphraseList> ParseFunnelKeyphrases(
    const base::Value::Dict& dict);

std::optional<PurchaseIntentFunnelSiteMap> ParseFunnelSites(
    const SegmentList& segments,
    const base::Value::Dict& dict);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_RESOURCE_PURCHASE_INTENT_VALUE_UTIL_H_
