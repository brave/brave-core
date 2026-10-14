// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_match_util.h"

#include <utility>

#include "brave/components/gestures/core/common/custom/custom_gesture_match_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_match_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_match_util.h"
#include "brave/components/gestures/core/common/gesture_match_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_match_util.h"

namespace gestures {

namespace {

// Floating-point noise from resampling shouldn't tip which candidate wins;
// only a real difference in match quality should.
constexpr double kDirectionalPreferenceEpsilon = 1e-6;

}  // namespace

GestureMatchInfo MatchGesture(
    const SmartGestureMapping& smart_gesture_mapping,
    base::optional_ref<const SmartGestureContentInfo> smart_gesture_content,
    const DirectionalGestureMapping& directional_gesture_mapping,
    const CustomGestureRecognizer& custom_gesture_recognizer,
    const DirectionalGestureStrokes& strokes,
    const GesturePath& gesture_path) {
  if (std::optional<GestureActions> smart_gesture_actions =
          MaybeMatchSmartGesture(smart_gesture_mapping, smart_gesture_content,
                                 gesture_path)) {
    return {.match_type = GestureMatchType::kSmart,
            .actions = std::move(*smart_gesture_actions)};
  }

  std::optional<DirectionalGestureMatchInfo> directional_gesture_match =
      MaybeMatchDirectionalGesture(directional_gesture_mapping, strokes,
                                   gesture_path);

  std::optional<CustomGestureMatchInfo> custom_gesture_match =
      custom_gesture_recognizer.MaybeMatchGesture(gesture_path);

  // Custom wins if directional has no match, or if custom is clearly closer
  // than directional beyond the epsilon margin.
  const bool custom_gesture_wins =
      custom_gesture_match &&
      (!directional_gesture_match ||
       custom_gesture_match->path_distance + kDirectionalPreferenceEpsilon <
           directional_gesture_match->distance);
  if (custom_gesture_wins) {
    return {.match_type = GestureMatchType::kCustom,
            .actions = custom_gesture_match->gesture->actions};
  }

  if (!directional_gesture_match) {
    return {};
  }

  return {.match_type = GestureMatchType::kDirectional,
          .actions = std::move(directional_gesture_match->actions)};
}

}  // namespace gestures
