/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "brave/browser/ui/focus_mode/focus_mode_prefs.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"

namespace {

class TestObserver : public FocusModeController::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  void OnFocusModeToggled(bool enabled) override {
    ++call_count_;
    last_enabled_ = enabled;
  }

  int call_count() const { return call_count_; }
  bool last_enabled() const { return last_enabled_; }

 private:
  int call_count_ = 0;
  bool last_enabled_ = false;
};

}  // namespace

class FocusModeControllerTest : public testing::Test {
 public:
  void SetUp() override { focus_mode::RegisterProfilePrefs(prefs_.registry()); }

 protected:
  TestingPrefServiceSimple prefs_;
  ui::UnownedUserDataHost user_data_host_;
};

TEST_F(FocusModeControllerTest, DefaultsToDisabled) {
  FocusModeController controller(user_data_host_, prefs_);
  EXPECT_FALSE(controller.IsEnabled());
}

TEST_F(FocusModeControllerTest, SetEnabledNotifiesObserversOnChange) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);

  controller.SetEnabled(true);

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());
  EXPECT_TRUE(observer.last_enabled());

  controller.SetEnabled(false);

  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(2, observer.call_count());
  EXPECT_FALSE(observer.last_enabled());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, SetEnabledIsNoOpWhenUnchanged) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);

  controller.SetEnabled(false);
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(0, observer.call_count());

  controller.SetEnabled(true);
  EXPECT_EQ(1, observer.call_count());

  controller.SetEnabled(true);
  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, ToggleEnabledFlipsStateAndNotifies) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);

  controller.ToggleEnabled();
  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());
  EXPECT_TRUE(observer.last_enabled());

  controller.ToggleEnabled();
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(2, observer.call_count());
  EXPECT_FALSE(observer.last_enabled());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, RemovedObserverIsNotNotified) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);
  controller.RemoveObserver(&observer);

  controller.SetEnabled(true);

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(0, observer.call_count());
}

TEST_F(FocusModeControllerTest, NotifiesAllRegisteredObservers) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer_a;
  TestObserver observer_b;
  controller.AddObserver(&observer_a);
  controller.AddObserver(&observer_b);

  controller.SetEnabled(true);

  EXPECT_EQ(1, observer_a.call_count());
  EXPECT_TRUE(observer_a.last_enabled());
  EXPECT_EQ(1, observer_b.call_count());
  EXPECT_TRUE(observer_b.last_enabled());

  controller.RemoveObserver(&observer_a);
  controller.RemoveObserver(&observer_b);
}

TEST_F(FocusModeControllerTest, SuppressWhileEnabledDisablesAndRestores) {
  FocusModeController controller(user_data_host_, prefs_);
  controller.SetEnabled(true);

  TestObserver observer;
  controller.AddObserver(&observer);

  {
    auto suppression = controller.Suppress();
    EXPECT_FALSE(controller.IsEnabled());
    EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeEnabled));
    EXPECT_EQ(1, observer.call_count());
    EXPECT_FALSE(observer.last_enabled());
  }

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(2, observer.call_count());
  EXPECT_TRUE(observer.last_enabled());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, SuppressWhileDisabledDoesNotNotify) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);

  {
    auto suppression = controller.Suppress();
    EXPECT_FALSE(controller.IsEnabled());
    EXPECT_EQ(0, observer.call_count());
  }

  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(0, observer.call_count());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, NestedSuppressionRestoresOnlyWhenAllReleased) {
  FocusModeController controller(user_data_host_, prefs_);
  controller.SetEnabled(true);

  TestObserver observer;
  controller.AddObserver(&observer);

  auto suppression_a = controller.Suppress();
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());
  EXPECT_FALSE(observer.last_enabled());

  auto suppression_b = controller.Suppress();
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());

  suppression_a.RunAndReset();
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());

  suppression_b.RunAndReset();
  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(2, observer.call_count());
  EXPECT_TRUE(observer.last_enabled());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, ToggleWhileSuppressedAppliesOnRelease) {
  FocusModeController controller(user_data_host_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);

  {
    auto suppression = controller.Suppress();
    controller.ToggleEnabled();
    EXPECT_FALSE(controller.IsEnabled());
    EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeEnabled));
    EXPECT_EQ(0, observer.call_count());
  }

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(1, observer.call_count());
  EXPECT_TRUE(observer.last_enabled());

  {
    auto suppression = controller.Suppress();
    EXPECT_EQ(2, observer.call_count());
    EXPECT_FALSE(observer.last_enabled());

    controller.ToggleEnabled();
    EXPECT_FALSE(controller.IsEnabled());
    EXPECT_FALSE(prefs_.GetBoolean(focus_mode::kFocusModeEnabled));
    EXPECT_EQ(2, observer.call_count());
  }

  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_EQ(2, observer.call_count());

  controller.RemoveObserver(&observer);
}

TEST_F(FocusModeControllerTest, InitializesFromPrefAndSyncsAcrossControllers) {
  prefs_.SetBoolean(focus_mode::kFocusModeEnabled, true);

  ui::UnownedUserDataHost host_a;
  FocusModeController controller_a(host_a, prefs_);
  EXPECT_TRUE(controller_a.IsEnabled());

  ui::UnownedUserDataHost host_b;
  FocusModeController controller_b(host_b, prefs_);
  EXPECT_TRUE(controller_b.IsEnabled());

  TestObserver observer_a;
  TestObserver observer_b;
  controller_a.AddObserver(&observer_a);
  controller_b.AddObserver(&observer_b);

  controller_a.SetEnabled(false);
  EXPECT_FALSE(prefs_.GetBoolean(focus_mode::kFocusModeEnabled));
  EXPECT_FALSE(controller_a.IsEnabled());
  EXPECT_FALSE(controller_b.IsEnabled());
  EXPECT_EQ(1, observer_a.call_count());
  EXPECT_FALSE(observer_a.last_enabled());
  EXPECT_EQ(1, observer_b.call_count());
  EXPECT_FALSE(observer_b.last_enabled());

  controller_b.ToggleEnabled();
  EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeEnabled));
  EXPECT_TRUE(controller_a.IsEnabled());
  EXPECT_TRUE(controller_b.IsEnabled());
  EXPECT_EQ(2, observer_a.call_count());
  EXPECT_TRUE(observer_a.last_enabled());
  EXPECT_EQ(2, observer_b.call_count());
  EXPECT_TRUE(observer_b.last_enabled());

  controller_a.RemoveObserver(&observer_a);
  controller_b.RemoveObserver(&observer_b);
}

TEST_F(FocusModeControllerTest, SuppressOnlyAffectsSuppressedController) {
  ui::UnownedUserDataHost host_a;
  FocusModeController controller_a(host_a, prefs_);
  ui::UnownedUserDataHost host_b;
  FocusModeController controller_b(host_b, prefs_);

  controller_a.SetEnabled(true);
  EXPECT_TRUE(controller_a.IsEnabled());
  EXPECT_TRUE(controller_b.IsEnabled());

  auto suppression = controller_a.Suppress();
  EXPECT_FALSE(controller_a.IsEnabled());
  EXPECT_TRUE(controller_b.IsEnabled());

  controller_b.SetEnabled(false);
  EXPECT_FALSE(controller_a.IsEnabled());
  EXPECT_FALSE(controller_b.IsEnabled());

  suppression.RunAndReset();
  EXPECT_FALSE(controller_a.IsEnabled());
  EXPECT_FALSE(controller_b.IsEnabled());
}
