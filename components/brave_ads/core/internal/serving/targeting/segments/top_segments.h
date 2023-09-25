/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_SEGMENTS_TOP_SEGMENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_SEGMENTS_TOP_SEGMENTS_H_

#include <string>

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

SegmentList GetTopSegments(const SegmentList& segments,
                           int max_count,
                           bool parent_only);

absl::optional<std::string> GetTopSegment(const SegmentList& segments,
                                          bool parent_only);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_SEGMENTS_TOP_SEGMENTS_H_
