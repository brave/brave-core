/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <string>
#include <vector>

#include "bat/ads/internal/purchase_intent/funnel_site_info.h"

namespace ads {

FunnelSiteInfo::FunnelSiteInfo() = default;

FunnelSiteInfo::FunnelSiteInfo(
    const PurchaseIntentSegmentList& segments,
    const std::string& url_netloc,
    const uint16_t weight)
    : segments(segments),
      url_netloc(url_netloc),
      weight(weight) {}

FunnelSiteInfo::FunnelSiteInfo(
    const FunnelSiteInfo& info) = default;

FunnelSiteInfo::~FunnelSiteInfo() = default;

}  // namespace ads
