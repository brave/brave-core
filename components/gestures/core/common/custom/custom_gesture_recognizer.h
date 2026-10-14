// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_

#include <optional>
#include <string_view>

#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Matches a freeform traced gesture_path against user-drawn custom gestures by
// hit count, not average distance; not rotation-invariant.
class CustomGestureRecognizer final {
 public:
  CustomGestureRecognizer();

  CustomGestureRecognizer(const CustomGestureRecognizer&) = delete;
  CustomGestureRecognizer& operator=(const CustomGestureRecognizer&) = delete;

  ~CustomGestureRecognizer();

  // Adds a user-drawn gesture to the set matched against in
  // `MaybeMatchGesture`.
  void Add(CustomGestureInfo custom_gesture);

  // Removes the custom gesture with the given `id`, e.g. when the user
  // removes it from the settings UI.
  void Remove(std::string_view id);

  void Clear();

  // Best match within the threshold, with distance for `GestureDrawSession`
  // to compare too. `nullopt` if none qualify.
  std::optional<CustomGestureMatchInfo> MaybeMatchGesture(
      const GesturePath& gesture_path) const;

  // Exposed so the settings UI can list, and warn about conflicts between,
  // the currently-saved custom gestures.
  const CustomGestures& custom_gestures() const { return custom_gestures_; }

 private:
  CustomGestures custom_gestures_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_RECOGNIZER_H_
