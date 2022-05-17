/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_TARGETING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_TARGETING_H_

#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace targeting {

struct UserModelInfo;

SegmentList GetTopChildSegments(const UserModelInfo& user_model);

SegmentList GetTopParentSegments(const UserModelInfo& user_model);

SegmentList GetTopChildInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopParentInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopChildLatentInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopParentLatentInterestSegments(const UserModelInfo& user_model);

SegmentList GetTopChildPurchaseIntentSegments(const UserModelInfo& user_model);

SegmentList GetTopParentPurchaseIntenSegments(const UserModelInfo& user_model);

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_TARGETING_H_
