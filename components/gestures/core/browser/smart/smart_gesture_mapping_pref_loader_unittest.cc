// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/smart/smart_gesture_mapping_pref_loader.h"

#include "base/values.h"
#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class SmartGestureMappingPrefLoaderTest : public testing::Test {
 public:
  SmartGestureMappingPrefLoaderTest() {
    RegisterProfilePrefs(pref_service_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  SmartGestureMapping mapping_;
};

TEST_F(SmartGestureMappingPrefLoaderTest, ShouldSkipPrefEntryThatIsNotAList) {
  pref_service_.SetDict(prefs::kSmartGestures,
                        base::DictValue().Set("link:right", "not a list"));

  LoadSmartGestureMapping(pref_service_, mapping_);

  // The malformed entry is skipped, leaving the built-in default in place.
  EXPECT_THAT(
      mapping_.ActionsFor(SmartGestureContentType::kLink,
                          SmartGestureDirectionType::kRight),
      testing::ElementsAre(GestureActionInfo{.command = kCopyLinkAddress}));
}

TEST_F(SmartGestureMappingPrefLoaderTest,
       ShouldSkipPrefEntryThatParsesToNoValidActions) {
  pref_service_.SetDict(
      prefs::kSmartGestures,
      base::DictValue().Set("link:right",
                            base::ListValue().Append(base::DictValue())));

  LoadSmartGestureMapping(pref_service_, mapping_);

  EXPECT_THAT(
      mapping_.ActionsFor(SmartGestureContentType::kLink,
                          SmartGestureDirectionType::kRight),
      testing::ElementsAre(GestureActionInfo{.command = kCopyLinkAddress}));
}

TEST_F(SmartGestureMappingPrefLoaderTest,
       ShouldOverlayValidPrefEntryOntoDefaults) {
  pref_service_.SetDict(
      prefs::kSmartGestures,
      base::DictValue().Set("link:right",
                            base::ListValue().Append(
                                base::DictValue().Set("command", kCopyImage))));

  LoadSmartGestureMapping(pref_service_, mapping_);

  EXPECT_THAT(mapping_.ActionsFor(SmartGestureContentType::kLink,
                                  SmartGestureDirectionType::kRight),
              testing::ElementsAre(GestureActionInfo{.command = kCopyImage}));
}

}  // namespace gestures
