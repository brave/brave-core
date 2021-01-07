/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_H_
#define BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_H_

#include <string>
#include <vector>

namespace ads {

using SegmentList = std::vector<std::string>;

SegmentList DeserializeSegments(
    const std::string& json);

std::string SerializeSegments(
    const SegmentList& segments);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_SEGMENT_H_
