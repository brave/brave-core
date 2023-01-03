/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_UTIL_H_

#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::targeting {

struct UserModelInfo;

SegmentList GetTopSegments(const SegmentList& segments,
                           int max_count,
                           bool parent_only);

SegmentList GetTopSegments(const UserModelInfo& user_model,
                           int max_count,
                           bool parent_only);

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_UTIL_H_
