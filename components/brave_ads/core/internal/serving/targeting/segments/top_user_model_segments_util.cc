/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_user_model_segments_util.h"

#include "base/containers/extend.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/segments/top_segments.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

namespace brave_ads {

SegmentList GetTopSegments(const UserModelInfo& user_model,
                           size_t max_count,
                           bool parent_only) {
  SegmentList segments;
  segments.reserve(3 * max_count);

  base::Extend(segments, GetTopSegments(user_model.intent.segments, max_count,
                                        parent_only));

  base::Extend(segments, GetTopSegments(user_model.latent_interest.segments,
                                        max_count, parent_only));

  base::Extend(segments, GetTopSegments(user_model.interest.segments, max_count,
                                        parent_only));

  return segments;
}

}  // namespace brave_ads
