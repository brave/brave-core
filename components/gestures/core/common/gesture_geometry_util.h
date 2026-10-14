// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_GEOMETRY_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_GEOMETRY_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Total distance traveled along `gesture_path`, summing each consecutive
// segment; unlike net start-to-end displacement, this doesn't shrink back
// toward zero as a shape curves back near its own start.
float GesturePathLength(const GesturePath& gesture_path);

// Simplifies `gesture_path` down to its significant turning points. A point
// is kept only once movement reverses away from it by at least
// `min_reversal_distance`, so tiny jitter-scale back-and-forth (e.g. a mouse
// bouncing between two pixels) never creates a turn. The kept point is the
// actual extremum, the farthest point reached along the direction being
// traveled before the reversal, not the later point where the reversal
// first became detectable, so a short, sharp excursion survives even if
// it's a small fraction of `gesture_path`'s overall length. Unlike
// directional stroke classification, this has no minimum-displacement or
// direction-octant requirement of its own, so a short excursion that never
// travels far enough to commit as a directional stroke is still preserved
// here. Call this before resampling a path for custom gesture matching (see
// `ResampleAndNormalizePath`), so a fixed, low point count doesn't
// interpolate straight past a meaningful but brief turn.
GesturePath RetainSignificantGeometry(const GesturePath& gesture_path,
                                      double min_reversal_distance);

// Resamples `gesture_path` to exactly `point_count` evenly-spaced points,
// then normalizes it for position and scale, so neither drawing speed nor
// size nor placement affect comparing it against another path.
GesturePath ResampleAndNormalizePath(const GesturePath& gesture_path,
                                     size_t point_count);

// A shape is closed (e.g. a circle or triangle) when its own start and end
// are close relative to its own length, so where the user happened to start
// drawing is arbitrary. Open shapes (e.g. a checkmark) have a meaningful
// start and end, so this intentionally doesn't apply to them.
bool IsClosedPath(const GesturePath& gesture_path);

// Distance from `normalized_path` to `normalized_candidate_path`, trying
// every rotation of the candidate's points, in both its recorded and
// reversed winding direction, if it's a closed shape, and keeping the best
// (smallest) result. Both must already be resampled to the same point
// count and normalized, e.g. via `ResampleAndNormalizePath`.
float DistanceToCandidate(const GesturePath& normalized_path,
                          const GesturePath& normalized_candidate_path,
                          bool candidate_is_closed);

// The highest hit score (see below) `normalized_candidate_path` can reach
// against `normalized_path` over every rotation and winding direction, if
// it's a closed shape. Runs its own alignment search rather than reusing
// `DistanceToCandidate`'s, since the rotation that minimizes average
// distance isn't necessarily the one that maximizes hit score.
float BestHitScore(const GesturePath& normalized_path,
                   const GesturePath& normalized_candidate_path,
                   bool candidate_is_closed,
                   float per_point_threshold);

// Average, over all of `normalized_path`'s points, of a per-point score
// that's 1 for a point landing exactly on its counterpart in
// `normalized_candidate_path`, falls off linearly to 0 by
// `per_point_threshold` away, and stays 0 beyond that. Unlike
// `DistanceToCandidate`'s plain mean, a handful of badly-diverging points
// can't be outvoted by many near-identical ones, since each point's
// contribution is capped at 0 once it's far enough off, however far beyond
// that it actually is. Exposed mainly for `BestHitScore`'s own alignment
// search; most callers want `BestHitScore` instead, since it already
// accounts for a closed shape's arbitrary starting point and winding.
float HitScore(const GesturePath& normalized_path,
               const GesturePath& normalized_candidate_path,
               float per_point_threshold);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_GEOMETRY_UTIL_H_
