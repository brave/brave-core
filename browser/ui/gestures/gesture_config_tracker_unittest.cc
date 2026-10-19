// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_config_tracker.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/browser/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_types.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureConfigTrackerTest : public testing::Test {
 public:
  GestureConfigTrackerTest() {
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
  }

 protected:
  GestureConfigTracker MakeConfigTracker() {
    return GestureConfigTracker(pref_service_);
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(GestureConfigTrackerTest, IsAllowedForHostIsTrueByDefault) {
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("example.com"));
}

TEST_F(GestureConfigTrackerTest,
       IsAllowedForHostIsFalseWhenGesturesAreDisabled) {
  pref_service_.SetBoolean(prefs::kEnabled, false);
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("example.com"));
}

TEST_F(GestureConfigTrackerTest, IsAllowedForHostIsFalseForDisallowedHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("example.com"));
}

TEST_F(GestureConfigTrackerTest,
       IsAllowedForHostIsFalseWhenHostIsSubdomainOfDisallowedHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("www.example.com"));
}

TEST_F(GestureConfigTrackerTest,
       IsAllowedForHostIsTrueForHostThatOnlyEndsWithDisallowedHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  // "notexample.com" ends with "example.com" but isn't a subdomain of it
  // (no dot boundary), so it must not be treated as disallowed.
  EXPECT_TRUE(config_tracker.IsAllowedForHost("notexample.com"));
}

TEST_F(GestureConfigTrackerTest,
       IsAllowedForHostMatchesDisallowedHostStoredWithMixedCase) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("Example.com", true));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("example.com"));
}

TEST_F(GestureConfigTrackerTest,
       IsAllowedForHostIgnoresDisallowedSitesEntrySetToFalse) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", false));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("example.com"));
}

TEST_F(GestureConfigTrackerTest, RockerSettingsDefaultToBackAndForward) {
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseRockerGesturesEnabled());
  EXPECT_THAT(config_tracker.mouse_rocker_left_actions(),
              testing::ElementsAre(
                  testing::Field(&GestureActionInfo::command,
                                 test::FakeMouseRockerDefaultCommands().left)));
  EXPECT_THAT(config_tracker.mouse_rocker_right_actions(),
              testing::ElementsAre(testing::Field(
                  &GestureActionInfo::command,
                  test::FakeMouseRockerDefaultCommands().right)));
}

TEST_F(GestureConfigTrackerTest, RockerSettingsUpdateLiveWhenPrefsChange) {
  GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetBoolean(prefs::kMouseRockerGesturesEnabled, false);

  EXPECT_FALSE(config_tracker.IsMouseRockerGesturesEnabled());
}

TEST_F(GestureConfigTrackerTest,
       WheelSettingsDefaultToTheRightButtonAndTabSwitching) {
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseWheelGesturesEnabled());
  EXPECT_EQ(MouseWheelModifierButtonType::kRight,
            config_tracker.MouseWheelModifierButton());
  EXPECT_THAT(config_tracker.mouse_wheel_up_actions(),
              testing::ElementsAre(
                  testing::Field(&GestureActionInfo::command,
                                 test::FakeMouseWheelDefaultCommands().up)));
  EXPECT_THAT(config_tracker.mouse_wheel_down_actions(),
              testing::ElementsAre(
                  testing::Field(&GestureActionInfo::command,
                                 test::FakeMouseWheelDefaultCommands().down)));
}

TEST_F(GestureConfigTrackerTest, WheelSettingsUpdateLiveWhenPrefsChange) {
  GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetInteger(
      prefs::kMouseWheelModifierButton,
      static_cast<int>(MouseWheelModifierButtonType::kMiddle));

  EXPECT_EQ(MouseWheelModifierButtonType::kMiddle,
            config_tracker.MouseWheelModifierButton());
}

TEST_F(GestureConfigTrackerTest,
       DirectionalGesturesOverrideJustTheRemappedKeyAndKeepOtherDefaults) {
  pref_service_.SetDict(
      prefs::kDirectionalGestures,
      base::DictValue().Set("↓", base::ListValue().Append(base::DictValue().Set(
                                     "command", IDC_BACK))));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_THAT(config_tracker.directional_gesture_mapping().ActionsForStrokes(
                  {DirectionalGestureDirectionType::kDown}),
              testing::ElementsAre(
                  testing::Field(&GestureActionInfo::command, IDC_BACK)));
  // The default for "↓→" is untouched by the "↓" override set in
  // `prefs::kDirectionalGestures`.
  EXPECT_THAT(config_tracker.directional_gesture_mapping().ActionsForStrokes(
                  {DirectionalGestureDirectionType::kDown,
                   DirectionalGestureDirectionType::kRight}),
              testing::ElementsAre(
                  testing::Field(&GestureActionInfo::command, IDC_CLOSE_TAB)));
}

TEST_F(GestureConfigTrackerTest, CustomGesturesAreParsedFromThePref) {
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "my-custom-gesture")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));
  GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_THAT(config_tracker.custom_gesture_recognizer().custom_gestures(),
              testing::ElementsAre(
                  testing::Field(&CustomGestureInfo::id, "my-custom-gesture")));
}

}  // namespace gestures
