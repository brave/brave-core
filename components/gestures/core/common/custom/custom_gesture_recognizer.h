// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_

#include <string_view>
#include <vector>

#include "base/types/optional_ref.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures {

// Matches a freeform traced path against user-recorded custom gestures. Both
// are resampled to the same point count and normalized for position and
// scale, so neither drawing speed nor size nor placement affect the match,
// then compared by average per-point distance. The closest one under the
// match threshold wins. Matching is not rotation-invariant, so a shape drawn
// at a different orientation than it was recorded will not match.
class CustomGestureRecognizer final {
 public:
  CustomGestureRecognizer();

  CustomGestureRecognizer(const CustomGestureRecognizer&) = delete;
  CustomGestureRecognizer& operator=(const CustomGestureRecognizer&) = delete;

  ~CustomGestureRecognizer();

  // Adds a user-recorded gesture to the set matched against in
  // `ActionsForMatch`.
  void Add(CustomGestureInfo custom_gesture);

  // Removes the custom gesture with the given `id`, e.g. when the user
  // deletes it from the settings UI.
  void Remove(std::string_view id);

  // Discards every recorded custom gesture.
  void Clear();

  // Returns the actions of the best-matching custom gesture if its distance
  // to `path` is within the match threshold, or an empty vector otherwise,
  // including when `path` has fewer than two points.
  const std::vector<GestureActionInfo>& ActionsForMatch(
      const std::vector<gfx::PointF>& path) const;

  // Exposed so the settings UI can list, and warn about conflicts between,
  // the currently-recorded custom gestures.
  const std::vector<CustomGestureInfo>& custom_gestures() const {
    return custom_gestures_;
  }

 private:
  // Returns the best-matching custom gesture within the match threshold.
  base::optional_ref<const CustomGestureInfo> MatchCustomGesture(
      const std::vector<gfx::PointF>& path) const;

  std::vector<CustomGestureInfo> custom_gestures_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
