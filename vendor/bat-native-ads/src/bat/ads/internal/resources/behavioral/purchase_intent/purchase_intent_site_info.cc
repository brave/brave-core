/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_site_info.h"

namespace ads {
namespace targeting {

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo() = default;

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo(const SegmentList& segments,
                                               const GURL& url_netloc,
                                               const uint16_t weight)
    : segments(segments), url_netloc(url_netloc), weight(weight) {}

PurchaseIntentSiteInfo::PurchaseIntentSiteInfo(
    const PurchaseIntentSiteInfo& info) = default;

PurchaseIntentSiteInfo& PurchaseIntentSiteInfo::operator=(
    const PurchaseIntentSiteInfo& info) = default;

PurchaseIntentSiteInfo::~PurchaseIntentSiteInfo() = default;

bool PurchaseIntentSiteInfo::operator==(
    const PurchaseIntentSiteInfo& rhs) const {
  return segments == rhs.segments && url_netloc == rhs.url_netloc &&
         weight == rhs.weight;
}

bool PurchaseIntentSiteInfo::operator!=(
    const PurchaseIntentSiteInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace targeting
}  // namespace ads
