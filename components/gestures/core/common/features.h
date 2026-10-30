// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_FEATURES_H_

#include <cstddef>

#include "base/component_export.h"
#include "base/feature.h"
#include "base/metrics/field_trial_params.h"  // IWYU pragma: keep
#include "base/time/time.h"

namespace gestures::features {

COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE(kGesturesFeature);

// Minimum distance a new traced point must be from the last accepted one,
// rejecting jitter before it reaches the gesture_path.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kGestureSessionMinPointDistancePixels);

// How long the mouse button, trackpad button, or touchscreen contact can be
// held with no qualifying movement before the gesture is abandoned.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(base::TimeDelta, kGestureSessionIdleHoldTimeout);

// How often to poll for `kGestureSessionIdleHoldTimeout` having elapsed
// while held with no qualifying movement.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(base::TimeDelta,
                           kGestureSessionIdleHoldPollInterval);

// Minimum straight-line movement, in pixels, to register as a directional
// stroke. Only seeds `kDirectionalGestureSensitivityInPixels`'s default.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kDirectionalGestureMinStrokePixels);

// Minimum ratio of a candidate stroke's straight-line displacement to the
// actual distance traveled, for it to commit as a directional stroke.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kDirectionalGestureMinStrokeStraightness);

// Maximum directional strokes retained per gesture; further strokes are
// discarded. Custom gestures aren't capped.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(size_t, kMaxDirectionalGestureStrokes);

// Number of points a custom gesture's gesture_path is resampled to before
// comparison against saved custom gestures.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(size_t, kCustomGestureResamplePointCount);

// Maximum average per-point distance, in normalized [0,1] space, for a
// traced gesture_path and a saved custom gesture to be considered a match.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureMaxMatchDistance);

// Maximum per-point distance, in normalized [0,1] space, counted as a "hit"
// when picking the saved custom gesture a traced gesture_path most resembles.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureInlierPointThreshold);

// A custom gesture is treated as closed (its own start point is arbitrary)
// when its start-to-end gap is smaller than this fraction of its own length.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureClosedPathGapRatio);

// Minimum hit_score, in [0,1], a custom gesture candidate must reach to be
// matched, even if its distance is within `kCustomGestureMaxMatchDistance`.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureMinHitScore);

// Minimum distance a draw over a link/image/selection must travel before it
// resolves as a smart gesture; below it, a barely-moved cursor can't commit.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kSmartGestureMinDistancePixels);

// Maximum distance a draw over a link/image/selection can travel and still
// resolve as a smart gesture; beyond it, falls through to directional/custom.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kSmartGestureMaxDistancePixels);

// Minimum accumulated scroll distance to register as a wheel gesture. Only
// seeds `prefs::kMouseWheelSensitivityInPixels`'s default.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kMinMouseWheelGestureDelta);

}  // namespace gestures::features

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_FEATURES_H_
