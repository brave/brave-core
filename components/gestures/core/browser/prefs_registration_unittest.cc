// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/prefs_registration.h"

#include "brave/components/gestures/core/browser/input/mouse/mouse_rocker_default_commands_info.h"
#include "brave/components/gestures/core/browser/input/mouse/mouse_wheel_default_commands_info.h"
#include "brave/components/gestures/core/browser/input/mouse/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_types.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

bool IsSyncable(sync_preferences::TestingPrefServiceSyncable& prefs,
                std::string_view pref_name) {
  return (prefs.registry()->GetRegistrationFlags(pref_name) &
          user_prefs::PrefRegistrySyncable::SYNCABLE_PREF) != 0;
}

int PrimaryActionCommand(sync_preferences::TestingPrefServiceSyncable& prefs,
                         std::string_view pref_name) {
  const base::DictValue& dict = prefs.GetList(pref_name).front().GetDict();
  return dict.FindInt("command").value();
}

}  // namespace

class PrefsRegistrationTest : public testing::Test {
 public:
  PrefsRegistrationTest() {
    RegisterProfilePrefs(prefs_.registry(),
                         test::FakeMouseRockerDefaultCommands(),
                         test::FakeMouseWheelDefaultCommands());
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(PrefsRegistrationTest, EnabledDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kEnabled));
}

TEST_F(PrefsRegistrationTest, EnabledIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(prefs_, prefs::kEnabled));
}

TEST_F(PrefsRegistrationTest, ShowTrailDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kShowTrail));
}

TEST_F(PrefsRegistrationTest, ShowTrailIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(prefs_, prefs::kShowTrail));
}

TEST_F(PrefsRegistrationTest, ShowMatchedGestureDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kShowMatchedGesture));
}

TEST_F(PrefsRegistrationTest, ShowMatchedGestureIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(prefs_, prefs::kShowMatchedGesture));
}

TEST_F(PrefsRegistrationTest, SensitivityDefaultsToTheFeatureParam) {
  EXPECT_EQ(prefs_.GetDouble(prefs::kDirectionalGestureSensitivityInPixels),
            features::kDirectionalGestureMinStrokePixels.Get());
}

TEST_F(PrefsRegistrationTest, SensitivityIsNotSyncable) {
  EXPECT_FALSE(
      IsSyncable(prefs_, prefs::kDirectionalGestureSensitivityInPixels));
}

TEST_F(PrefsRegistrationTest, DirectionalGesturesAreSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kDirectionalGestures));
}

TEST_F(PrefsRegistrationTest, CustomGesturesAreSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kCustomGestures));
}

TEST_F(PrefsRegistrationTest, RockerGesturesEnabledDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kMouseRockerGesturesEnabled));
}

TEST_F(PrefsRegistrationTest, RockerLeftActionDefaultsToCallerSuppliedValue) {
  EXPECT_EQ(test::FakeMouseRockerDefaultCommands().left,
            PrimaryActionCommand(prefs_, prefs::kMouseRockerLeftAction));
}

TEST_F(PrefsRegistrationTest, RockerRightActionDefaultsToCallerSuppliedValue) {
  EXPECT_EQ(test::FakeMouseRockerDefaultCommands().right,
            PrimaryActionCommand(prefs_, prefs::kMouseRockerRightAction));
}

TEST_F(PrefsRegistrationTest, RockerLeftActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kMouseRockerLeftAction));
}

TEST_F(PrefsRegistrationTest, RockerRightActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kMouseRockerRightAction));
}

TEST_F(PrefsRegistrationTest, WheelGesturesEnabledDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kMouseWheelGesturesEnabled));
}

TEST_F(PrefsRegistrationTest, WheelModifierButtonDefaultsToRight) {
  EXPECT_EQ(static_cast<int>(MouseWheelModifierButtonType::kRight),
            prefs_.GetInteger(prefs::kMouseWheelModifierButton));
}

TEST_F(PrefsRegistrationTest, WheelUpActionDefaultsToCallerSuppliedValue) {
  EXPECT_EQ(test::FakeMouseWheelDefaultCommands().up,
            PrimaryActionCommand(prefs_, prefs::kMouseWheelUpAction));
}

TEST_F(PrefsRegistrationTest, WheelDownActionDefaultsToCallerSuppliedValue) {
  EXPECT_EQ(test::FakeMouseWheelDefaultCommands().down,
            PrimaryActionCommand(prefs_, prefs::kMouseWheelDownAction));
}

TEST_F(PrefsRegistrationTest, WheelUpActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kMouseWheelUpAction));
}

TEST_F(PrefsRegistrationTest, WheelDownActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(prefs_, prefs::kMouseWheelDownAction));
}

}  // namespace gestures
