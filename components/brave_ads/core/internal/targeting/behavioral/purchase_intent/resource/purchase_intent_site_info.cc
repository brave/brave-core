/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_site_info.h"

#include <tuple>
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

bool PurchaseIntentSiteInfo::operator==(
    const PurchaseIntentSiteInfo& other) const {
  const auto tie = [](const PurchaseIntentSiteInfo& purchase_intent_site) {
    return std::tie(purchase_intent_site.segments,
                    purchase_intent_site.url_netloc,
                    purchase_intent_site.weight);
  };

  return tie(*this) == tie(other);
}

bool PurchaseIntentSiteInfo::operator!=(
    const PurchaseIntentSiteInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
