/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_funnel_info.h"

#include <utility>

namespace brave_ads {

PurchaseIntentFunnelInfo::PurchaseIntentFunnelInfo() = default;

PurchaseIntentFunnelInfo::PurchaseIntentFunnelInfo(SegmentList segments,
                                                   int weight)
    : segments(std::move(segments)), weight(weight) {}

PurchaseIntentFunnelInfo::PurchaseIntentFunnelInfo(
    const PurchaseIntentFunnelInfo& other) = default;

PurchaseIntentFunnelInfo& PurchaseIntentFunnelInfo::operator=(
    const PurchaseIntentFunnelInfo& other) = default;

PurchaseIntentFunnelInfo::PurchaseIntentFunnelInfo(
    PurchaseIntentFunnelInfo&& other) noexcept = default;

PurchaseIntentFunnelInfo& PurchaseIntentFunnelInfo::operator=(
    PurchaseIntentFunnelInfo&& other) noexcept = default;

PurchaseIntentFunnelInfo::~PurchaseIntentFunnelInfo() = default;

}  // namespace brave_ads
