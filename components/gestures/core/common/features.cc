// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/features.h"

namespace gestures::features {

BASE_FEATURE(kGesturesFeature, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(double,
                   kGestureSessionMinPointDistancePixels,
                   &kGesturesFeature,
                   2.0);
BASE_FEATURE_PARAM(base::TimeDelta,
                   kGestureSessionIdleHoldTimeout,
                   &kGesturesFeature,
                   base::Seconds(3));
BASE_FEATURE_PARAM(base::TimeDelta,
                   kGestureSessionIdleHoldPollInterval,
                   &kGesturesFeature,
                   base::Milliseconds(250));
BASE_FEATURE_PARAM(double,
                   kSmartGestureMaxDistancePixels,
                   &kGesturesFeature,
                   80.0);

BASE_FEATURE_PARAM(double,
                   kDirectionalGestureMinStrokePixels,
                   &kGesturesFeature,
                   30.0);
BASE_FEATURE_PARAM(double,
                   kDirectionalGestureMinStrokeStraightness,
                   &kGesturesFeature,
                   0.9);
BASE_FEATURE_PARAM(size_t, kMaxDirectionalGestureStrokes, &kGesturesFeature, 4);

BASE_FEATURE_PARAM(size_t,
                   kCustomGestureResamplePointCount,
                   &kGesturesFeature,
                   128);
BASE_FEATURE_PARAM(double,
                   kCustomGestureMaxMatchDistance,
                   &kGesturesFeature,
                   0.14);
BASE_FEATURE_PARAM(double,
                   kCustomGestureInlierPointThreshold,
                   &kGesturesFeature,
                   0.1);
BASE_FEATURE_PARAM(double,
                   kCustomGestureClosedPathGapRatio,
                   &kGesturesFeature,
                   0.2);

BASE_FEATURE_PARAM(double, kMinMouseWheelGestureDelta, &kGesturesFeature, 0.0);

}  // namespace gestures::features
