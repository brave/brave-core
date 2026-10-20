/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"

#include "base/callback_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_prefs.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
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

class FocusModeControllerTest : public testing::Test {
 public:
  void SetUp() override { focus_mode::RegisterProfilePrefs(prefs_.registry()); }

 protected:
  TestingPrefServiceSimple prefs_;
  testing::NiceMock<MockBrowserWindowInterface> browser_window_;
};

TEST_F(FocusModeControllerTest, DefaultsToDisabled) {
  FocusModeController controller(browser_window_, prefs_);
  EXPECT_FALSE(controller.IsEnabled());
}

TEST_F(FocusModeControllerTest, SetEnabledNotifiesObserversOnChange) {
  FocusModeController controller(browser_window_, prefs_);
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
  FocusModeController controller(browser_window_, prefs_);
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
  FocusModeController controller(browser_window_, prefs_);
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
  FocusModeController controller(browser_window_, prefs_);
  TestObserver observer;
  controller.AddObserver(&observer);
  controller.RemoveObserver(&observer);

  controller.SetEnabled(true);

  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_EQ(0, observer.call_count());
}

TEST_F(FocusModeControllerTest, NotifiesAllRegisteredObservers) {
  FocusModeController controller(browser_window_, prefs_);
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

TEST_F(FocusModeControllerTest, InitializesFromPrefAndUpdatesOnSetEnabled) {
  prefs_.SetBoolean(focus_mode::kFocusModeLastWindowState, true);

  FocusModeController controller(browser_window_, prefs_);
  EXPECT_TRUE(controller.IsEnabled());

  controller.SetEnabled(false);
  EXPECT_FALSE(controller.IsEnabled());
  EXPECT_FALSE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));

  controller.ToggleEnabled();
  EXPECT_TRUE(controller.IsEnabled());
  EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));
}

TEST_F(FocusModeControllerTest, DidBecomeActiveSyncsPrefWithWindowState) {
  testing::NiceMock<MockBrowserWindowInterface> browser_a;
  BrowserWindowInterface::DidBecomeActiveCallback activate_a;
  ON_CALL(browser_a, RegisterDidBecomeActive(testing::_))
      .WillByDefault(
          [&](BrowserWindowInterface::DidBecomeActiveCallback callback) {
            activate_a = std::move(callback);
            return base::CallbackListSubscription();
          });

  testing::NiceMock<MockBrowserWindowInterface> browser_b;
  BrowserWindowInterface::DidBecomeActiveCallback activate_b;
  ON_CALL(browser_b, RegisterDidBecomeActive(testing::_))
      .WillByDefault(
          [&](BrowserWindowInterface::DidBecomeActiveCallback callback) {
            activate_b = std::move(callback);
            return base::CallbackListSubscription();
          });

  FocusModeController controller_a(browser_a, prefs_);
  controller_a.SetEnabled(true);
  EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));

  FocusModeController controller_b(browser_b, prefs_);
  EXPECT_TRUE(controller_b.IsEnabled());

  controller_b.SetEnabled(false);
  EXPECT_FALSE(controller_b.IsEnabled());
  EXPECT_TRUE(controller_a.IsEnabled());
  EXPECT_FALSE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));

  activate_a.Run(&browser_a);
  EXPECT_TRUE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));

  activate_b.Run(&browser_b);
  EXPECT_FALSE(prefs_.GetBoolean(focus_mode::kFocusModeLastWindowState));
}
