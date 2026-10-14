// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Matches a freeform traced path against user-recorded custom gestures. Both
// are resampled to the same point count and normalized for position and
// scale, so neither drawing speed nor size nor placement affect the match.
// The one with the most points landing close enough to count as a hit wins
// (see `kCustomGestureInlierPointThreshold`), not necessarily the one with
// the smallest average distance; its own average distance, not the hit
// count, is still what decides whether it's under the match threshold.
// Matching is not rotation-invariant, so a shape drawn at a different
// orientation than it was recorded will not match.
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

  // Actions of the best match within the threshold (see
  // `BestCustomGestureMatch`), or empty if none qualify.
  const GestureActions& ActionsForMatch(const GesturePath& gesture_path) const;

  // Best match and its distance even above the threshold, since
  // `GestureDrawSession` compares it against a directional stroke's own.
  CustomGestureMatchInfo MatchCustomGesture(
      const GesturePath& gesture_path) const;

  // Like `MatchCustomGesture`, but only among candidates within the match
  // threshold, so a would-be-accepted candidate can't lose to one that
  // scores higher but would be rejected anyway.
  CustomGestureMatchInfo BestCustomGestureMatch(
      const GesturePath& gesture_path) const;

  // Exposed so the settings UI can list, and warn about conflicts between,
  // the currently-recorded custom gestures.
  const CustomGestures& custom_gestures() const { return custom_gestures_; }

 private:
  CustomGestures custom_gestures_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
