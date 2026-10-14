// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/test/gesture_action_dispatcher_test_util.h"

#include <optional>
#include <utility>

#include "base/test/test_future.h"
#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures::test {

void RunDispatcherAndWait(GestureActionDispatcher& action_dispatcher,
                          GestureActions actions) {
  base::test::TestFuture<void> on_all_actions_done;
  action_dispatcher.DispatchActionsAsync(std::move(actions),
                                         /*smart_gesture_content=*/std::nullopt,
                                         on_all_actions_done.GetCallback());
  ASSERT_TRUE(on_all_actions_done.Wait());
}

}  // namespace gestures::test
