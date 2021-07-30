/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_H_

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

namespace ads {

class AdType;

namespace p2a {

void RecordAdOpportunityForSegments(const AdType& ad_type,
                                    const SegmentList& segments);

}  // namespace p2a
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_P2A_P2A_AD_OPPORTUNITIES_P2A_AD_OPPORTUNITY_H_
