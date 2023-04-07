/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads::targeting {

struct UserModelInfo;

UserModelInfo BuildUserModel(const SegmentList& interest_segments,
                             const SegmentList& latent_interest_segments,
                             const SegmentList& purchase_intent_segments);

}  // namespace brave_ads::targeting

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_BUILDER_UNITTEST_UTIL_H_
