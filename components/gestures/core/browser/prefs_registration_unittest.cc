// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/prefs_registration.h"

#include "brave/components/gestures/core/browser/mouse_rocker_default_commands.h"
#include "brave/components/gestures/core/browser/mouse_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/browser/test/fake_mouse_rocker_wheel_default_commands.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/mouse/wheel/mouse_wheel_modifier_button_types.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

bool IsSyncable(sync_preferences::TestingPrefServiceSyncable* prefs,
                std::string_view path) {
  return (prefs->registry()->GetRegistrationFlags(path) &
          user_prefs::PrefRegistrySyncable::SYNCABLE_PREF) != 0;
}

int FirstActionCommand(sync_preferences::TestingPrefServiceSyncable* prefs,
                       std::string_view path) {
  const base::DictValue& dict = prefs->GetList(path).front().GetDict();
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
  EXPECT_FALSE(IsSyncable(&prefs_, prefs::kEnabled));
}

TEST_F(PrefsRegistrationTest, ShowTrailDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kShowTrail));
}

TEST_F(PrefsRegistrationTest, ShowTrailIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(&prefs_, prefs::kShowTrail));
}

TEST_F(PrefsRegistrationTest, ShowMatchedGestureDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kShowMatchedGesture));
}

TEST_F(PrefsRegistrationTest, ShowMatchedGestureIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(&prefs_, prefs::kShowMatchedGesture));
}

TEST_F(PrefsRegistrationTest, SensitivityDefaultsToTheFeatureParam) {
  EXPECT_EQ(prefs_.GetDouble(prefs::kSensitivityInPixels),
            features::kMinStrokePixels.Get());
}

TEST_F(PrefsRegistrationTest, SensitivityIsNotSyncable) {
  EXPECT_FALSE(IsSyncable(&prefs_, prefs::kSensitivityInPixels));
}

TEST_F(PrefsRegistrationTest, DirectionalGesturesAreSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kDirectionalGestures));
}

TEST_F(PrefsRegistrationTest, CustomGesturesAreSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kCustomGestures));
}

TEST_F(PrefsRegistrationTest, RockerGesturesEnabledDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kMouseRockerGesturesEnabled));
}

TEST_F(PrefsRegistrationTest, RockLeftActionDefaultsToTheCallerSuppliedLeft) {
  EXPECT_EQ(test::FakeMouseRockerDefaultCommands().left,
            FirstActionCommand(&prefs_, prefs::kMouseRockerLeftAction));
}

TEST_F(PrefsRegistrationTest, RockRightActionDefaultsToTheCallerSuppliedRight) {
  EXPECT_EQ(test::FakeMouseRockerDefaultCommands().right,
            FirstActionCommand(&prefs_, prefs::kMouseRockerRightAction));
}

TEST_F(PrefsRegistrationTest, RockLeftActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kMouseRockerLeftAction));
}

TEST_F(PrefsRegistrationTest, RockRightActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kMouseRockerRightAction));
}

TEST_F(PrefsRegistrationTest, WheelGesturesEnabledDefaultsToTrue) {
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kMouseWheelGesturesEnabled));
}

TEST_F(PrefsRegistrationTest, WheelModifierButtonDefaultsToRight) {
  EXPECT_EQ(static_cast<int>(MouseWheelModifierButtonType::kRight),
            prefs_.GetInteger(prefs::kMouseWheelModifierButton));
}

TEST_F(PrefsRegistrationTest, WheelUpActionDefaultsToTheCallerSuppliedUp) {
  EXPECT_EQ(test::FakeMouseWheelDefaultCommands().up,
            FirstActionCommand(&prefs_, prefs::kMouseWheelUpAction));
}

TEST_F(PrefsRegistrationTest, WheelDownActionDefaultsToTheCallerSuppliedDown) {
  EXPECT_EQ(test::FakeMouseWheelDefaultCommands().down,
            FirstActionCommand(&prefs_, prefs::kMouseWheelDownAction));
}

TEST_F(PrefsRegistrationTest, WheelUpActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kMouseWheelUpAction));
}

TEST_F(PrefsRegistrationTest, WheelDownActionIsSyncable) {
  EXPECT_TRUE(IsSyncable(&prefs_, prefs::kMouseWheelDownAction));
}

}  // namespace gestures
