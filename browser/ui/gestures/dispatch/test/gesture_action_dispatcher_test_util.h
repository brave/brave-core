// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_GESTURE_ACTION_DISPATCHER_TEST_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_GESTURE_ACTION_DISPATCHER_TEST_UTIL_H_

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"

namespace gestures {

class GestureActionDispatcher;

namespace test {

// Dispatches `actions` and blocks until they have actually run, not just
// been posted.
void RunDispatcherAndWait(GestureActionDispatcher& action_dispatcher,
                          GestureActions actions);

// As above, but with captured smart-gesture content the actions can read
// instead of a live fetch.
void RunDispatcherAndWait(GestureActionDispatcher& action_dispatcher,
                          GestureActions actions,
                          SmartGestureContentInfo smart_gesture_content);

}  // namespace test

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_TEST_GESTURE_ACTION_DISPATCHER_TEST_UTIL_H_
