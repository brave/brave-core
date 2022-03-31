/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/data_types/behavioral/purchase_intent/purchase_intent_funnel_keyword_info.h"

namespace ads {
namespace ad_targeting {

PurchaseIntentFunnelKeywordInfo::PurchaseIntentFunnelKeywordInfo() = default;

PurchaseIntentFunnelKeywordInfo::PurchaseIntentFunnelKeywordInfo(
    const std::string& keywords,
    const uint16_t weight)
    : keywords(keywords), weight(weight) {}

PurchaseIntentFunnelKeywordInfo::PurchaseIntentFunnelKeywordInfo(
    const PurchaseIntentFunnelKeywordInfo& info) = default;

PurchaseIntentFunnelKeywordInfo::~PurchaseIntentFunnelKeywordInfo() = default;

}  // namespace ad_targeting
}  // namespace ads
