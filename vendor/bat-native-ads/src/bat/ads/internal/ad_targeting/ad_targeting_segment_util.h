/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_UTIL_H_
#define BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_UTIL_H_

#include <string>
#include <vector>

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

namespace ads {

std::vector<std::string> SplitSegment(
    const std::string& segment);

std::string GetParentSegment(
    const std::string& segment);

SegmentList GetParentSegments(
    const SegmentList& segments);

bool ShouldFilterSegment(
    const std::string& segment);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_UTIL_H_
