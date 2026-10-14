// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point_f.h"

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

  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "foo.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowHostWhenGesturesAreDisabled) {
  pref_service_.SetBoolean(prefs::kEnabled, false);
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "foo.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowConfiguredHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("foo.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "foo.com"));
}

TEST_F(GestureConfigTrackerTest, ShouldDisallowSubdomainsOfConfiguredHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("foo.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_FALSE(config_tracker.IsAllowedForHost("https", "www.foo.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowHostWithSimilarSuffixToDisallowedHost) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("foo.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  // "notfoo.com" ends with "foo.com" but isn't a subdomain of it
  // (no dot boundary), so it must not be treated as disallowed.
  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "notfoo.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldAllowHostWhenDisallowedSitesEntryIsDisabled) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("foo.com", false));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "foo.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldDisallowHostWhenAddressAndSchemeMatchDisallowedSitesEntry) {
  pref_service_.SetDict(prefs::kDisallowedSites,
                        base::DictValue().Set("brave://settings", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  // A committed brave:// page's actual scheme is chrome, per
  // chromium_src's `MaybeRewriteVirtualURL`.
  EXPECT_FALSE(config_tracker.IsAllowedForHost("chrome", "settings"));
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
                        base::DictValue().Set("https://foo.com", true));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  // A scheme-qualified entry matches only its exact host, since it means
  // one specific page, not "this site" the way a bare hostname does.
  EXPECT_TRUE(config_tracker.IsAllowedForHost("https", "www.foo.com"));
}

TEST_F(GestureConfigTrackerTest,
       ShouldPreserveOtherDirectionalDefaultsWhenOneIsRemapped) {
  pref_service_.SetDict(
      prefs::kDirectionalGestures,
      base::DictValue().Set("↓", base::ListValue().Append(base::DictValue().Set(
                                     "command", IDC_BACK))));
  const GestureConfigTracker config_tracker = MakeConfigTracker();
  const DirectionalGestureMapping& directional_gesture_mapping =
      config_tracker.directional_gesture_mapping();

  EXPECT_THAT(directional_gesture_mapping.ActionsForStrokes(
                  {DirectionalGestureStrokeType::kDown}),
              testing::ElementsAre(MakeGestureAction(IDC_BACK)));
  // The default for "↓→" is untouched by the "↓" override set in
  // `prefs::kDirectionalGestures`.
  EXPECT_THAT(directional_gesture_mapping.ActionsForStrokes(
                  {DirectionalGestureStrokeType::kDown,
                   DirectionalGestureStrokeType::kRight}),
              testing::ElementsAre(MakeGestureAction(IDC_CLOSE_TAB)));
}

TEST_F(GestureConfigTrackerTest, ShouldLoadCustomGesturesFromPreferences) {
  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "1792208323001")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  EXPECT_THAT(
      config_tracker.custom_gesture_recognizer().custom_gestures(),
      testing::ElementsAre(testing::FieldsAre(
          /*id=*/"1792208323001",
          /*gesture_path=*/
          testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(1, 1)),
          /*actions=*/testing::ElementsAre(MakeGestureAction(IDC_BACK)))));
}

TEST_F(GestureConfigTrackerTest,
       ShouldUpdateCustomGesturesWhenPreferencesChange) {
  const GestureConfigTracker config_tracker = MakeConfigTracker();

  pref_service_.SetList(
      prefs::kCustomGestures,
      base::ListValue().Append(
          base::DictValue()
              .Set("id", "1792208323001")
              .Set("points",
                   base::ListValue()
                       .Append(base::DictValue().Set("x", 0.0).Set("y", 0.0))
                       .Append(base::DictValue().Set("x", 1.0).Set("y", 1.0)))
              .Set("actions", base::ListValue().Append(base::DictValue().Set(
                                  "command", IDC_BACK)))));

  EXPECT_THAT(
      config_tracker.custom_gesture_recognizer().custom_gestures(),
      testing::ElementsAre(testing::FieldsAre(
          /*id=*/"1792208323001",
          /*gesture_path=*/
          testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(1, 1)),
          /*actions=*/testing::ElementsAre(MakeGestureAction(IDC_BACK)))));
}

}  // namespace gestures
