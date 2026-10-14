// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_geometry_util.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_heading_util.h"
#include "brave/components/gestures/core/common/gesture_path_resample_util.h"
#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace gestures {

float GesturePathLength(const GesturePath& gesture_path) {
  float length = 0;
  std::optional<gfx::PointF> previous_point;
  for (const gfx::PointF& point : gesture_path) {
    if (previous_point) {
      length += (point - *previous_point).Length();
    }
    previous_point = point;
  }
  return length;
}

GesturePath RetainSignificantGeometry(const GesturePath& gesture_path,
                                      double min_reversal_distance) {
  if (gesture_path.size() < 2) {
    return gesture_path;
  }

  GesturePath retained_path = {gesture_path.front()};

  // `heading` is fixed until the next commit resets it, so a path can't
  // steadily pivot through small steps without ever registering a reversal.
  gfx::PointF heading_origin = gesture_path.front();
  gfx::PointF extremum = gesture_path.front();
  std::optional<gfx::Vector2dF> heading;
  float max_forward = 0;

  for (size_t i = 1; i < gesture_path.size(); ++i) {
    const gfx::PointF& point = gesture_path.at(i);
    if (!heading) {
      StartGesturePathHeading(heading_origin, point, heading, max_forward);
      if (heading) {
        extremum = point;
      }
      continue;
    }

    // How far `point` projects along `heading` from `heading_origin`, split
    // into forward (progress) and perpendicular (sideways) components.
    const gfx::Vector2dF displacement = point - heading_origin;
    const auto forward =
        base::saturated_cast<float>(gfx::DotProduct(displacement, *heading));
    if (forward > max_forward) {
      max_forward = forward;
      extremum = point;
      continue;
    }

    // Not a new farthest point, but not necessarily a reversal, since a
    // growing perpendicular deviation counts as a turn too, not just
    // regression.
    const gfx::Vector2dF perpendicular =
        displacement - gfx::ScaleVector2d(*heading, forward);
    const float regression = max_forward - forward;
    if (regression < min_reversal_distance &&
        perpendicular.Length() < min_reversal_distance) {
      continue;
    }

    // Preserves `extremum` itself, not `point`, so a brief excursion
    // survives even as a small fraction of the whole path's length.
    retained_path.push_back(extremum);
    heading_origin = extremum;
    heading.reset();
    StartGesturePathHeading(heading_origin, point, heading, max_forward);
    if (heading) {
      extremum = point;
    }
  }

  if (retained_path.back() != gesture_path.back()) {
    retained_path.push_back(gesture_path.back());
  }
  return retained_path;
}

GesturePath ResampleAndNormalizePath(const GesturePath& gesture_path,
                                     size_t point_count) {
  return NormalizeGesturePathScaleAndTranslation(
      ResampleGesturePathToCount(gesture_path, point_count));
}

bool IsClosedPath(const GesturePath& gesture_path) {
  const float length = GesturePathLength(gesture_path);
  if (length == 0) {
    return false;
  }
  const float gap = (gesture_path.back() - gesture_path.front()).Length();
  return gap / length < features::kCustomGestureClosedPathGapRatio.Get();
}

GesturePath BestAlignedCandidate(const GesturePath& normalized_path,
                                 const GesturePath& normalized_candidate_path,
                                 bool candidate_is_closed) {
  if (!candidate_is_closed) {
    return normalized_candidate_path;
  }

  // A closed shape has no meaningful winding direction either, so a
  // clockwise-drawn gesture must match a counter-clockwise saved one (and
  // vice versa) just as readily as it matches one traced in the same
  // direction. Rotating alone only changes the starting point, not the
  // direction, so the reversed point order is tried as well.
  GesturePath reversed_candidate_path;
  reversed_candidate_path.reserve(normalized_candidate_path.size());
  std::ranges::reverse_copy(normalized_candidate_path,
                            std::back_inserter(reversed_candidate_path));

  float best_distance = std::numeric_limits<float>::max();
  GesturePath best_aligned_candidate = normalized_candidate_path;
  for (const GesturePath& candidate_path :
       {normalized_candidate_path, reversed_candidate_path}) {
    for (size_t offset = 0; offset < candidate_path.size(); ++offset) {
      GesturePath rotated_candidate = RotateGesturePath(candidate_path, offset);
      const float distance =
          AverageGesturePathDistance(normalized_path, rotated_candidate);
      if (distance < best_distance) {
        best_distance = distance;
        best_aligned_candidate = std::move(rotated_candidate);
      }
    }
  }
  return best_aligned_candidate;
}

float DistanceToCandidate(const GesturePath& normalized_path,
                          const GesturePath& normalized_candidate_path,
                          bool candidate_is_closed) {
  return AverageGesturePathDistance(
      normalized_path,
      BestAlignedCandidate(normalized_path, normalized_candidate_path,
                           candidate_is_closed));
}

float HitScore(const GesturePath& normalized_path,
               const GesturePath& normalized_candidate_path,
               float per_point_threshold) {
  if (normalized_path.empty() || per_point_threshold <= 0) {
    return 0;
  }
  float total_score = 0;
  for (size_t i = 0; i < normalized_path.size(); ++i) {
    const float distance =
        (normalized_path.at(i) - normalized_candidate_path.at(i)).Length();
    // A point right on top of its counterpart scores a full point; one at
    // or beyond `per_point_threshold` scores nothing; in between falls off
    // linearly, so a near-perfect point still counts for more than a
    // barely-within-threshold one, rather than both counting as an
    // identical binary hit.
    total_score += std::max(0.0F, 1.0F - distance / per_point_threshold);
  }
  return total_score / base::saturated_cast<float>(normalized_path.size());
}

float BestHitScore(const GesturePath& normalized_path,
                   const GesturePath& normalized_candidate_path,
                   bool candidate_is_closed,
                   float per_point_threshold) {
  if (!candidate_is_closed) {
    return HitScore(normalized_path, normalized_candidate_path,
                    per_point_threshold);
  }

  // The rotation/winding that best aligns a closed candidate by average
  // distance (see `BestAlignedCandidate`) isn't necessarily the one that
  // best aligns it by hit score, so this runs its own independent search
  // rather than reusing that alignment.
  GesturePath reversed_candidate_path;
  reversed_candidate_path.reserve(normalized_candidate_path.size());
  std::ranges::reverse_copy(normalized_candidate_path,
                            std::back_inserter(reversed_candidate_path));

  float best_hit_score = 0;
  for (const GesturePath& candidate_path :
       {normalized_candidate_path, reversed_candidate_path}) {
    for (size_t offset = 0; offset < candidate_path.size(); ++offset) {
      const GesturePath rotated_candidate =
          RotateGesturePath(candidate_path, offset);
      best_hit_score = std::max(
          best_hit_score,
          HitScore(normalized_path, rotated_candidate, per_point_threshold));
    }
  }
  return best_hit_score;
}

}  // namespace gestures
