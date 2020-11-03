/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/site_info.h"

namespace ads {
namespace ad_targeting {
namespace behavioral {

SiteInfo::SiteInfo() = default;

SiteInfo::SiteInfo(
    const PurchaseIntentSegmentList& segments,
    const std::string& url_netloc,
    const uint16_t weight)
    : segments(segments),
      url_netloc(url_netloc),
      weight(weight) {}

SiteInfo::SiteInfo(
    const SiteInfo& info) = default;

SiteInfo::~SiteInfo() = default;

bool SiteInfo::operator==(
    const SiteInfo& rhs) const {
  return segments == rhs.segments &&
      url_netloc == rhs.url_netloc &&
      weight == rhs.weight;
}

bool SiteInfo::operator!=(
    const SiteInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace behavioral
}  // namespace ad_targeting
}  // namespace ads
