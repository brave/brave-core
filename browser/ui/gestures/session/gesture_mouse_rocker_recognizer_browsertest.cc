// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/gesture_mouse_rocker_recognizer.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/session/gesture_config_tracker.h"
#include "brave/browser/ui/gestures/session/test/web_mouse_event_test_util.h"
#include "brave/browser/ui/gestures/test/gesture_mouse_rocker_test_util.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"

namespace gestures {

class GestureMouseRockerRecognizerBrowserTest : public PlatformBrowserTest {
 protected:
  // `action_dispatcher_` holds a `raw_ref` to the active tab, and
  // `config_tracker_` holds a `raw_ref` to the profile's `PrefService`;
  // browser teardown destroys both before this fixture's own members are
  // destroyed, so dropping them here, while the tab and profile are still
  // alive, avoids releasing a dangling ref later.
  void TearDownOnMainThread() override {
    action_dispatcher_.reset();
    config_tracker_.reset();
    PlatformBrowserTest::TearDownOnMainThread();
  }

  GestureMouseRockerRecognizer MakeMouseRockerRecognizer(
      bool is_allowed_for_site = true) {
    return {config_tracker(), action_dispatcher(),
            base::BindRepeating([](bool allowed) { return allowed; },
                                is_allowed_for_site),
            base::BindRepeating(
                &GestureMouseRockerRecognizerBrowserTest::OnRockerGestureFired,
                base::Unretained(this))};
  }

  PrefService& GetPrefs() {
    return *chrome_test_utils::GetProfile(this)->GetPrefs();
  }

  GestureConfigTracker& config_tracker() {
    if (!config_tracker_) {
      config_tracker_ = std::make_unique<GestureConfigTracker>(GetPrefs());
    }
    return *config_tracker_;
  }

  tabs::TabInterface& active_tab() {
    return *chrome_test_utils::GetActiveTab(this);
  }

  GestureActionDispatcher& action_dispatcher() {
    if (!action_dispatcher_) {
      action_dispatcher_ =
          std::make_unique<GestureActionDispatcher>(active_tab());
    }
    return *action_dispatcher_;
  }

  void OnRockerGestureFired() { ++rocker_gesture_fired_count_; }

  size_t rocker_gesture_fired_count_ = 0;

 private:
  std::unique_ptr<GestureConfigTracker> config_tracker_;
  std::unique_ptr<GestureActionDispatcher> action_dispatcher_;
};

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       ShouldConsumeRightClickWhenLeftButtonIsHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_TRUE(test::HandleMouseDown(
      mouse_rocker_recognizer,
      test::MakeWebMouseButtonEvent(blink::WebMouseEvent::Type::kMouseDown,
                                    blink::WebMouseEvent::Button::kRight,
                                    blink::WebInputEvent::kLeftButtonDown)));
  EXPECT_EQ(1U, rocker_gesture_fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       ShouldConsumeLeftClickWhenRightButtonIsHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_TRUE(test::HandleMouseDown(
      mouse_rocker_recognizer,
      test::MakeWebMouseButtonEvent(blink::WebMouseEvent::Type::kMouseDown,
                                    blink::WebMouseEvent::Button::kLeft,
                                    blink::WebInputEvent::kRightButtonDown)));
  EXPECT_EQ(1U, rocker_gesture_fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       ShouldIgnoreClickWhenNoOtherButtonIsHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_FALSE(test::HandleMouseDown(
      mouse_rocker_recognizer,
      test::MakeWebMouseButtonEvent(blink::WebMouseEvent::Type::kMouseDown,
                                    blink::WebMouseEvent::Button::kRight,
                                    blink::WebInputEvent::kNoModifiers)));
  EXPECT_EQ(0U, rocker_gesture_fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       ShouldIgnoreClicksWhenDisabled) {
  GetPrefs().SetBoolean(prefs::kMouseRockerGesturesEnabled, false);
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_FALSE(test::HandleMouseDown(
      mouse_rocker_recognizer,
      test::MakeWebMouseButtonEvent(blink::WebMouseEvent::Type::kMouseDown,
                                    blink::WebMouseEvent::Button::kRight,
                                    blink::WebInputEvent::kLeftButtonDown)));
  EXPECT_EQ(0U, rocker_gesture_fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       ShouldIgnoreClicksOnDisallowedSite) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer(/*is_allowed_for_site=*/false);

  EXPECT_FALSE(test::HandleMouseDown(
      mouse_rocker_recognizer,
      test::MakeWebMouseButtonEvent(blink::WebMouseEvent::Type::kMouseDown,
                                    blink::WebMouseEvent::Button::kRight,
                                    blink::WebInputEvent::kLeftButtonDown)));
  EXPECT_EQ(0U, rocker_gesture_fired_count_);
}

}  // namespace gestures
