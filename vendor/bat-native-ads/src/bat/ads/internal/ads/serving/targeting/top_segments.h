/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_H_

#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::targeting {

struct UserModelInfo;

SegmentList GetTopChildSegments(const UserModelInfo& user_model);
SegmentList GetTopParentSegments(const UserModelInfo& user_model);

SegmentList GetTopChildInterestSegments(const UserModelInfo& user_model);
SegmentList GetTopParentInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopChildLatentInterestSegments(const UserModelInfo& user_model);
SegmentList GetTopParentLatentInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopChildPurchaseIntentSegments(const UserModelInfo& user_model);
SegmentList GetTopParentPurchaseIntentSegments(const UserModelInfo& user_model);

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_TOP_SEGMENTS_H_
