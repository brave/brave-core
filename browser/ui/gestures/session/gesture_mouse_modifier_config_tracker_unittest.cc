// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_mouse_modifier_config_tracker.h"

#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureMouseModifierConfigTrackerTest : public testing::Test {
 public:
  GestureMouseModifierConfigTrackerTest() {
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
  }

 protected:
  GestureMouseModifierConfigTracker MakeConfigTracker() {
    return GestureMouseModifierConfigTracker(pref_service_);
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(GestureMouseModifierConfigTrackerTest,
       ShouldDefaultRockerActionsToBackAndForward) {
  const GestureMouseModifierConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseRockerGesturesEnabled());
  EXPECT_THAT(config_tracker.mouse_rocker_left_actions(),
              testing::ElementsAre(MakeGestureAction(
                  test::FakeMouseRockerDefaultCommands().left)));
  EXPECT_THAT(config_tracker.mouse_rocker_right_actions(),
              testing::ElementsAre(MakeGestureAction(
                  test::FakeMouseRockerDefaultCommands().right)));
}

TEST_F(GestureMouseModifierConfigTrackerTest,
       ShouldUpdateRockerSettingsWhenPreferencesChange) {
  const GestureMouseModifierConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetBoolean(prefs::kMouseRockerGesturesEnabled, false);

  EXPECT_FALSE(config_tracker.IsMouseRockerGesturesEnabled());
}

TEST_F(GestureMouseModifierConfigTrackerTest,
       ShouldDefaultWheelModifierToRightMouseButtonAndActionsToTabSwitching) {
  const GestureMouseModifierConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseWheelGesturesEnabled());
  EXPECT_EQ(MouseWheelModifierButtonType::kRight,
            config_tracker.MouseWheelModifierButton());
  EXPECT_THAT(config_tracker.mouse_wheel_up_actions(),
              testing::ElementsAre(
                  MakeGestureAction(test::FakeMouseWheelDefaultCommands().up)));
  EXPECT_THAT(config_tracker.mouse_wheel_down_actions(),
              testing::ElementsAre(MakeGestureAction(
                  test::FakeMouseWheelDefaultCommands().down)));
}

TEST_F(GestureMouseModifierConfigTrackerTest,
       ShouldUpdateWheelSettingsWhenPreferencesChange) {
  const GestureMouseModifierConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetInteger(
      prefs::kMouseWheelModifierButton,
      static_cast<int>(MouseWheelModifierButtonType::kMiddle));

  EXPECT_EQ(MouseWheelModifierButtonType::kMiddle,
            config_tracker.MouseWheelModifierButton());
}

}  // namespace gestures
