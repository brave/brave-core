/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "testing/gtest/include/gtest/gtest.h"

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

TEST(FocusModeControllerTest, DefaultsToDisabled) {
  FocusModeController controller;
  EXPECT_FALSE(controller.IsEnabled());
}

TEST(FocusModeControllerTest, SetEnabledNotifiesObserversOnChange) {
  FocusModeController controller;
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

TEST(FocusModeControllerTest, SetEnabledIsNoOpWhenUnchanged) {
  FocusModeController controller;
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

TEST(FocusModeControllerTest, ToggleEnabledFlipsStateAndNotifies) {
  FocusModeController controller;
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

TEST(FocusModeControllerTest, RemovedObserverIsNotNotified) {
  FocusModeController controller;
  TestObserver observer;
  controller.AddObserver(&observer);
  controller.RemoveObserver(&observer);

  controller.SetEnabled(true);

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(0, observer.call_count());
}

TEST(FocusModeControllerTest, NotifiesAllRegisteredObservers) {
  FocusModeController controller;
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
