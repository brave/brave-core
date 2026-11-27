// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/directional/directional_gesture_mapping_pref_loader.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class DirectionalGestureMappingPrefLoaderTest : public testing::Test {
 public:
  DirectionalGestureMappingPrefLoaderTest() {
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  DirectionalGestureMapping mapping_{test::kDefaultCommands};
};

TEST_F(DirectionalGestureMappingPrefLoaderTest,
       ShouldSkipPrefEntryThatIsNotAList) {
  pref_service_.SetDict(prefs::kDirectionalGestures,
                        base::DictValue().Set("←", "not a list"));

  LoadDirectionalGestureMapping(pref_service_, mapping_);

  // The malformed entry is skipped, leaving the built-in default in place.
  EXPECT_THAT(mapping_.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeBack)));
}

TEST_F(DirectionalGestureMappingPrefLoaderTest,
       ShouldSkipPrefEntryThatParsesToNoValidActions) {
  pref_service_.SetDict(
      prefs::kDirectionalGestures,
      base::DictValue().Set("←", base::ListValue().Append(base::DictValue())));

  LoadDirectionalGestureMapping(pref_service_, mapping_);

  EXPECT_THAT(mapping_.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeBack)));
}

TEST_F(DirectionalGestureMappingPrefLoaderTest,
       ShouldOverlayValidPrefEntryOntoDefaults) {
  pref_service_.SetDict(
      prefs::kDirectionalGestures,
      base::DictValue().Set("←", base::ListValue().Append(base::DictValue().Set(
                                     "command", test::kFakeNewTab))));

  LoadDirectionalGestureMapping(pref_service_, mapping_);

  EXPECT_THAT(mapping_.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeNewTab)));
}

}  // namespace gestures
