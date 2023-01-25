/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/choose/eligible_ads_predictor_util.h"

#include <iterator>

#include "base/ranges/algorithm.h"

namespace ads {

SegmentList SegmentIntersection(SegmentList lhs, SegmentList rhs) {
  base::ranges::sort(lhs);
  base::ranges::sort(rhs);

  SegmentList intersection;
  base::ranges::set_intersection(lhs, rhs, std::back_inserter(intersection));
  return intersection;
}

}  // namespace ads
