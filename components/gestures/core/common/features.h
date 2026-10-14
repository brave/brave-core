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
// rejecting jitter before it reaches the path matching/overlay are built from.
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

// Maximum total distance traveled for a draw starting over a link, image, or
// selection to still resolve as a smart gesture; beyond this, it falls
// through to the directional/custom cascade instead, so drawing a longer
// shape over that content isn't blocked by a short smart-gesture swipe.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kSmartGestureMaxDistancePixels);

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

// Number of points a custom gesture's path is resampled to before comparison
// against saved custom gestures.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(size_t, kCustomGestureResamplePointCount);

// Maximum average per-point distance, in normalized [0,1] space, for a
// traced path and a saved custom gesture to be considered a match.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureMaxMatchDistance);

// Maximum per-point distance, in normalized [0,1] space, for a point to
// count as a "hit" when picking which saved custom gesture a traced path
// most resembles. Deciding by hit fraction rather than average distance
// stops a shape that's a near-exact fit everywhere except a couple of
// distinctive points (e.g. a rocket's nose and fins) from losing out to a
// shape that's mediocre-but-never-great everywhere.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureInlierPointThreshold);

// A custom gesture is treated as closed (its own start point is arbitrary)
// when its start-to-end gap is smaller than this fraction of its own length.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureClosedPathGapRatio);

// Minimum accumulated scroll distance to register as a wheel gesture. Only
// seeds `prefs::kMouseWheelSensitivityInPixels`'s default.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kMinMouseWheelGestureDelta);

}  // namespace gestures::features

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_FEATURES_H_
