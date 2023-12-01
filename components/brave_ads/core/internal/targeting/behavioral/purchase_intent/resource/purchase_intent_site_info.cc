/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_site_info.h"

#include <utility>

namespace brave_ads {

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo() = default;

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo(SegmentList segments,
                                               GURL url_netloc,
                                               const uint16_t weight)
    : segments(std::move(segments)),
      url_netloc(std::move(url_netloc)),
      weight(weight) {}

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo(
    const PurchaseIntentSiteInfo& other) = default;

PurchaseIntentSiteInfo& PurchaseIntentSiteInfo::operator=(
    const PurchaseIntentSiteInfo& other) = default;

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo(
    PurchaseIntentSiteInfo&& other) noexcept = default;

PurchaseIntentSiteInfo& PurchaseIntentSiteInfo::operator=(
    PurchaseIntentSiteInfo&& other) noexcept = default;

PurchaseIntentSiteInfo::~PurchaseIntentSiteInfo() = default;

}  // namespace brave_ads
