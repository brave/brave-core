// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/touch_point_state_util.h"

namespace gestures {

bool IsActiveTouchState(blink::WebTouchPoint::State state) {
  switch (state) {
    case blink::WebTouchPoint::State::kStatePressed:
    case blink::WebTouchPoint::State::kStateMoved:
    case blink::WebTouchPoint::State::kStateStationary: {
      return true;
    }

    case blink::WebTouchPoint::State::kStateReleased:
    case blink::WebTouchPoint::State::kStateCancelled:
    case blink::WebTouchPoint::State::kStateUndefined: {
      return false;
    }
  }
}

}  // namespace gestures
