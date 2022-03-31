/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_

#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace ad_targeting {

struct UserModelInfo;

UserModelInfo BuildUserModel(const SegmentList& interest_segments,
                             const SegmentList& latent_interest_segments,
                             const SegmentList& purchase_intent_segments);

}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_AD_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_
