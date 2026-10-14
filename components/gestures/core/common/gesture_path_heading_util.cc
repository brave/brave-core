// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_heading_util.h"

namespace gestures {

void StartGesturePathHeading(gfx::PointF heading_origin,
                             gfx::PointF point,
                             std::optional<gfx::Vector2dF>& heading,
                             float& max_forward) {
  gfx::Vector2dF displacement = point - heading_origin;
  if (displacement.IsZero()) {
    heading.reset();
    return;
  }
  max_forward = displacement.Length();
  displacement.Normalize();
  heading = displacement;
}

}  // namespace gestures
