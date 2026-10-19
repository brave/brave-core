// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/features.h"

namespace gestures::features {

BASE_FEATURE(kGesturesFeature, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(double, kMinStrokePixels, &kGesturesFeature, 30.0);

BASE_FEATURE_PARAM(double, kMinWheelDelta, &kGesturesFeature, 0.0);

BASE_FEATURE_PARAM(size_t, kMaxDirectionalStrokes, &kGesturesFeature, 4);

BASE_FEATURE_PARAM(base::TimeDelta,
                   kIdleHoldTimeout,
                   &kGesturesFeature,
                   base::Seconds(3));

BASE_FEATURE_PARAM(base::TimeDelta,
                   kIdleHoldPollInterval,
                   &kGesturesFeature,
                   base::Milliseconds(250));

BASE_FEATURE_PARAM(size_t,
                   kCustomGestureResamplePointCount,
                   &kGesturesFeature,
                   32);

BASE_FEATURE_PARAM(double,
                   kCustomGestureMatchThreshold,
                   &kGesturesFeature,
                   0.12);

BASE_FEATURE_PARAM(double, kClosedPathGapRatio, &kGesturesFeature, 0.2);

}  // namespace gestures::features
