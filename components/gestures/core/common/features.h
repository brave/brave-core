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

// Minimum straight-line movement, in pixels, for it to register as a
// directional stroke. Only seeds `prefs::kSensitivityInPixels`'s registered
// default; once that pref has a value persisted for a profile (customized or
// not), a later change to this param has no effect on that profile.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kMinStrokePixels);

// Minimum accumulated scroll distance for it to register as a wheel
// gesture. Only seeds `prefs::kMouseWheelSensitivityInPixels`'s registered
// default; once that pref has a value persisted for a profile (customized or
// not), a later change to this param has no effect on that profile.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kMinWheelDelta);

// Maximum directional strokes retained per gesture; further strokes are
// discarded. Custom gestures aren't capped, since `CustomGestureRecognizer`
// matches the raw traced path instead.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(size_t, kMaxDirectionalStrokes);

// How long the mouse button, trackpad button, or touchscreen contact can be
// held with no qualifying movement before the gesture is considered abandoned.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(base::TimeDelta, kIdleHoldTimeout);

// How often to poll for `kIdleHoldTimeout` having elapsed while a mouse
// button, trackpad button, or touchscreen contact is held with no
// qualifying movement.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(base::TimeDelta, kIdleHoldPollInterval);

// Number of points a custom gesture's path is resampled to before comparison
// against saved custom gestures.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(size_t, kCustomGestureResamplePointCount);

// Average per-point distance, in normalized [0,1] space, below which a traced
// path and a saved custom gesture are considered a match.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kCustomGestureMatchThreshold);

// A custom gesture is treated as a closed shape (e.g. a circle or triangle,
// where the starting point is arbitrary) when the distance between its first
// and last point is smaller than this fraction of its own path length. Closed
// custom gestures are matched at every rotation of their point sequence, not
// just the one traced when the gesture was drawn.
COMPONENT_EXPORT(GESTURES_FEATURES)
BASE_DECLARE_FEATURE_PARAM(double, kClosedPathGapRatio);

}  // namespace gestures::features

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_FEATURES_H_
