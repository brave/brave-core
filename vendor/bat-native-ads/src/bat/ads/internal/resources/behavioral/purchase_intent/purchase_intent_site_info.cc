/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_site_info.h"

#include <utility>

namespace ads::targeting {

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

bool PurchaseIntentSiteInfo::operator==(
    const PurchaseIntentSiteInfo& other) const {
  return segments == other.segments && url_netloc == other.url_netloc &&
         weight == other.weight;
}

bool PurchaseIntentSiteInfo::operator!=(
    const PurchaseIntentSiteInfo& other) const {
  return !(*this == other);
}

}  // namespace ads::targeting
