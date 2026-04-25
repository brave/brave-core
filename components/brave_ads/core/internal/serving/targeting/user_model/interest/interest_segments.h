/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_SEGMENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_SEGMENTS_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

void BuildInterestSegments(BuildSegmentsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_USER_MODEL_INTEREST_INTEREST_SEGMENTS_H_
