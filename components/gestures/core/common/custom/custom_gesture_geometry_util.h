// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_GEOMETRY_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_GEOMETRY_UTIL_H_

#include <vector>

#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Resamples `path` to exactly `point_count` evenly-spaced points, then
// normalizes it for position and scale, so neither drawing speed nor size
// nor placement affect comparing it against another path.
std::vector<gfx::PointF> ResampleAndNormalizePath(
    const std::vector<gfx::PointF>& path,
    size_t point_count);

// A shape is closed (e.g. a circle or triangle) when its own start and end
// are close relative to its own length, so where the user happened to start
// drawing is arbitrary. Open shapes (e.g. a checkmark) have a meaningful
// start and end, so this intentionally doesn't apply to them.
bool IsClosedPath(const std::vector<gfx::PointF>& raw_points);

// Distance from `path` to `candidate`, trying every rotation of the
// candidate's points, in both its recorded and reversed winding direction,
// if it's a closed shape, and keeping the best (smallest) result. Both must
// already be resampled to the same point count and normalized, e.g. via
// `ResampleAndNormalizePath`.
float DistanceToCandidate(const std::vector<gfx::PointF>& normalized_path,
                          const std::vector<gfx::PointF>& normalized_candidate,
                          bool candidate_is_closed);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_GEOMETRY_UTIL_H_
