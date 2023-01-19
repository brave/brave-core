/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/models/behavioral/purchase_intent/purchase_intent_funnel_keyword_info.h"

#include <utility>

namespace ads::targeting {

PurchaseIntentFunnelKeywordInfo::PurchaseIntentFunnelKeywordInfo() = default;

PurchaseIntentFunnelKeywordInfo::PurchaseIntentFunnelKeywordInfo(
    std::string keywords,
    const uint16_t weight)
    : keywords(std::move(keywords)), weight(weight) {}

}  // namespace ads::targeting
