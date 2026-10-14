// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/browser/test/fake_mouse_rocker_wheel_default_commands.h"
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

TEST_F(GestureConfigTrackerTest, ShouldAllowHostByDefault) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "example.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowHostWhenGesturesAreDisabled) {
  pref_service_.SetBoolean(prefs::kEnabled, false);
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "example.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowConfiguredHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "example.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowSubdomainsOfConfiguredHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "www.example.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowHostWithSimilarSuffixToDisallowedHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  // "notexample.com" ends with "example.com" but isn't a subdomain of it
  // (no dot boundary), so it must not be treated as disallowed.
  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "notexample.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldMatchDisallowedHostsRegardlessOfLetterCase) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("Example.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "example.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowHostWhenDisallowedSitesEntryIsDisabled) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("example.com", false));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "example.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldDisallowHostWhenAddressAndSchemeMatchDisallowedSitesEntry) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("brave://settings", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("brave", "settings"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowHostWhenSchemeDiffersFromDisallowedSitesEntry) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("brave://settings", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "settings"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowSubdomainsOfSchemeSpecificDisallowedSitesEntry) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("https://example.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  // A scheme-qualified entry matches only its exact host, since it means
  // one specific page, not "this site" the way a bare hostname does.
  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "www.example.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDefaultRockerActionsToBackAndForward) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseRockerGesturesEnabled());
  EXPECT_THAT(config_tracker.mouse_rocker_left_actions(),
              testing::ElementsAre(testing::FieldsAre(
                  test::FakeMouseRockerDefaultCommands().left,
                  /*command_params=*/testing::_)));
  EXPECT_THAT(config_tracker.mouse_rocker_right_actions(),
              testing::ElementsAre(testing::FieldsAre(
                  test::FakeMouseRockerDefaultCommands().right,
                  /*command_params=*/testing::_)));
}

TEST_F(GestureConfigTrackerTest,
       ShouldUpdateRockerSettingsWhenPreferencesChange) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetBoolean(prefs::kMouseRockerGesturesEnabled, false);

  EXPECT_FALSE(config_tracker.IsMouseRockerGesturesEnabled());
}

TEST_F(GestureConfigTrackerTest,
       ShouldUseRightMouseButtonAndTabSwitchingAsWheelDefaults) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsMouseWheelGesturesEnabled());
  EXPECT_EQ(MouseWheelModifierButtonType::kRight,
            config_tracker.MouseWheelModifierButton());
  EXPECT_THAT(config_tracker.mouse_wheel_up_actions(),
              testing::ElementsAre(
                  testing::FieldsAre(test::FakeMouseWheelDefaultCommands().up,
                                     /*command_params=*/testing::_)));
  EXPECT_THAT(config_tracker.mouse_wheel_down_actions(),
              testing::ElementsAre(
                  testing::FieldsAre(test::FakeMouseWheelDefaultCommands().down,
                                     /*command_params=*/testing::_)));
}

TEST_F(GestureConfigTrackerTest,
       ShouldUpdateWheelSettingsWhenPreferencesChange) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetInteger(
      prefs::kMouseWheelModifierButton,
      static_cast<int>(MouseWheelModifierButtonType::kMiddle));

  EXPECT_EQ(MouseWheelModifierButtonType::kMiddle,
            config_tracker.MouseWheelModifierButton());
}

TEST_F(GestureConfigTrackerTest,
       ShouldPreserveOtherDirectionalDefaultsWhenOneIsRemapped) {
  pref_service_.SetDict(
      prefs::kDirectionalGestures,
      base::DictValue().Set("↓", base::ListValue().Append(base::DictValue().Set(
                                     "command", IDC_BACK))));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_THAT(config_tracker.directional_gesture_mapping().ActionsForStrokes(
                  {DirectionalGestureStrokeType::kDown}),
              testing::ElementsAre(
                  testing::FieldsAre(IDC_BACK, /*command_params=*/testing::_)));
  // The default for "↓→" is untouched by the "↓" override set in
  // `prefs::kDirectionalGestures`.
  EXPECT_THAT(config_tracker.directional_gesture_mapping().ActionsForStrokes(
                  {DirectionalGestureStrokeType::kDown,
                   DirectionalGestureStrokeType::kRight}),
              testing::ElementsAre(testing::FieldsAre(
                  IDC_CLOSE_TAB, /*command_params=*/testing::_)));
}

TEST_F(GestureConfigTrackerTest, ShouldLoadCustomGesturesFromPreferences) {
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "1700000000000")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_THAT(config_tracker.custom_gesture_recognizer().custom_gestures(),
              testing::ElementsAre(testing::FieldsAre("1700000000000",
                                                      /*path=*/testing::_,
                                                      /*actions=*/testing::_)));
}

TEST_F(GestureConfigTrackerTest,
       ShouldUpdateCustomGesturesWhenPreferencesChange) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "1700000000000")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));

  EXPECT_THAT(config_tracker.custom_gesture_recognizer().custom_gestures(),
              testing::ElementsAre(testing::FieldsAre("1700000000000",
                                                      /*path=*/testing::_,
                                                      /*actions=*/testing::_)));
}

}  // namespace gestures
