// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_geometry_util.h"

#include <algorithm>
#include <limits>
#include <optional>

#include "brave/components/gestures/core/common/features.h"

namespace gestures {

namespace {

float PathLength(const std::vector<gfx::PointF>& path) {
  float length = 0;
  std::optional<gfx::PointF> previous_point;
  for (const gfx::PointF& point : path) {
    if (previous_point) {
      length += (point - *previous_point).Length();
    }
    previous_point = point;
  }
  return length;
}

std::vector<gfx::PointF> ResampleToCount(const std::vector<gfx::PointF>& path,
                                         size_t point_count) {
  const float total_length = PathLength(path);
  if (total_length == 0) {
    return {point_count, path.front()};
  }

  const float interval = total_length / static_cast<float>(point_count - 1);
  std::vector<gfx::PointF> resampled_points;
  resampled_points.reserve(point_count);
  resampled_points.push_back(path.front());

  float accumulated = 0;
  for (size_t i = 1; i < path.size() && resampled_points.size() < point_count;
       ++i) {
    const gfx::PointF& previous = path.at(i - 1);
    const gfx::PointF& current = path.at(i);
    float segment_length = (current - previous).Length();

    while (accumulated + segment_length >= interval &&
           resampled_points.size() < point_count) {
      const float remaining = interval - accumulated;
      const float ratio = segment_length > 0 ? remaining / segment_length : 0;
      const gfx::PointF new_point(
          previous.x() + ratio * (current.x() - previous.x()),
          previous.y() + ratio * (current.y() - previous.y()));
      resampled_points.push_back(new_point);
      segment_length -= remaining;
      accumulated = 0;
    }
    accumulated += segment_length;
  }

  while (resampled_points.size() < point_count) {
    resampled_points.push_back(path.back());
  }
  return resampled_points;
}

std::vector<gfx::PointF> NormalizeScaleAndTranslation(
    const std::vector<gfx::PointF>& path) {
  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float max_y = std::numeric_limits<float>::lowest();
  for (const gfx::PointF& point : path) {
    min_x = std::min(min_x, point.x());
    min_y = std::min(min_y, point.y());
    max_x = std::max(max_x, point.x());
    max_y = std::max(max_y, point.y());
  }

  const float scale = std::max(max_x - min_x, max_y - min_y);

  std::vector<gfx::PointF> normalized;
  normalized.reserve(path.size());
  for (const gfx::PointF& point : path) {
    if (scale == 0) {
      normalized.emplace_back(0, 0);
    } else {
      normalized.emplace_back((point.x() - min_x) / scale,
                              (point.y() - min_y) / scale);
    }
  }
  return normalized;
}

float AverageDistance(const std::vector<gfx::PointF>& first_path,
                      const std::vector<gfx::PointF>& second_path) {
  float total = 0;
  for (size_t i = 0; i < first_path.size(); ++i) {
    total += (first_path.at(i) - second_path.at(i)).Length();
  }
  return total / static_cast<float>(first_path.size());
}

// Returns `path` with its points rotated so the point at `offset` comes
// first, preserving order. Lets a closed custom gesture be compared starting
// from any point along its perimeter, not just the one recorded when it was
// drawn.
std::vector<gfx::PointF> RotatePath(const std::vector<gfx::PointF>& path,
                                    size_t offset) {
  std::vector<gfx::PointF> rotated;
  rotated.reserve(path.size());
  for (size_t i = 0; i < path.size(); ++i) {
    rotated.push_back(path.at((i + offset) % path.size()));
  }
  return rotated;
}

}  // namespace

std::vector<gfx::PointF> ResampleAndNormalizePath(
    const std::vector<gfx::PointF>& path,
    size_t point_count) {
  return NormalizeScaleAndTranslation(ResampleToCount(path, point_count));
}

bool IsClosedPath(const std::vector<gfx::PointF>& raw_points) {
  const float length = PathLength(raw_points);
  if (length == 0) {
    return false;
  }
  const float gap = (raw_points.back() - raw_points.front()).Length();
  return gap / length < features::kClosedPathGapRatio.Get();
}

float DistanceToCandidate(const std::vector<gfx::PointF>& normalized_path,
                          const std::vector<gfx::PointF>& normalized_candidate,
                          bool candidate_is_closed) {
  if (!candidate_is_closed) {
    return AverageDistance(normalized_path, normalized_candidate);
  }

  // A closed shape has no meaningful winding direction either, so a
  // clockwise-drawn gesture must match a counter-clockwise saved one (and
  // vice versa) just as readily as it matches one traced in the same
  // direction. Rotating alone only changes the starting point, not the
  // direction, so the reversed point order is tried as well.
  std::vector<gfx::PointF> reversed_candidate(normalized_candidate.rbegin(),
                                              normalized_candidate.rend());

  float best_distance = std::numeric_limits<float>::max();
  for (const std::vector<gfx::PointF>& candidate :
       {normalized_candidate, reversed_candidate}) {
    for (size_t offset = 0; offset < candidate.size(); ++offset) {
      best_distance = std::min(
          best_distance,
          AverageDistance(normalized_path, RotatePath(candidate, offset)));
    }
  }
  return best_distance;
}

}  // namespace gestures
