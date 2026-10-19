// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_mouse_rocker_recognizer.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/gestures/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/gesture_config_tracker.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"

namespace gestures {

namespace {

blink::WebMouseEvent MakeMouseDown(blink::WebMouseEvent::Button button,
                                   int modifiers) {
  return {blink::WebMouseEvent::Type::kMouseDown,
          /*position=*/gfx::PointF(0, 0),
          /*global_position=*/gfx::PointF(0, 0),
          button,
          /*click_count_param=*/1,
          modifiers,
          base::TimeTicks::Now()};
}

// Bundles `HandleMouseDown`'s return value with the resulting
// `on_fired_callback` count, so each test can assert both outcomes in one
// call.
std::pair<bool, size_t> HandleMouseDownAndCountFires(
    GestureMouseRockerRecognizer& mouse_rocker_recognizer,
    const size_t& fired_count,
    const blink::WebMouseEvent& web_mouse_event) {
  const bool consumed =
      mouse_rocker_recognizer.HandleMouseDown(web_mouse_event);
  return {consumed, fired_count};
}

}  // namespace

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
    return {
        config_tracker(), action_dispatcher(),
        base::BindRepeating([](bool allowed) { return allowed; },
                            is_allowed_for_site),
        base::BindRepeating(&GestureMouseRockerRecognizerBrowserTest::OnFired,
                            base::Unretained(this))};
  }

  GestureConfigTracker& config_tracker() {
    if (!config_tracker_) {
      config_tracker_ = std::make_unique<GestureConfigTracker>(
          *chrome_test_utils::GetProfile(this)->GetPrefs());
    }
    return *config_tracker_;
  }

  GestureActionDispatcher& action_dispatcher() {
    if (!action_dispatcher_) {
      action_dispatcher_ = std::make_unique<GestureActionDispatcher>(
          *chrome_test_utils::GetActiveTab(this));
    }
    return *action_dispatcher_;
  }

  void OnFired() { ++fired_count_; }

  size_t fired_count_ = 0;

 private:
  std::unique_ptr<GestureConfigTracker> config_tracker_;
  std::unique_ptr<GestureActionDispatcher> action_dispatcher_;
};

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       HandleMouseDownConsumesRightClickWhileLeftIsHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_THAT(HandleMouseDownAndCountFires(
                  mouse_rocker_recognizer, fired_count_,
                  MakeMouseDown(blink::WebMouseEvent::Button::kRight,
                                blink::WebInputEvent::kLeftButtonDown)),
              testing::Pair(true, 1));
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       HandleMouseDownConsumesLeftClickWhileRightIsHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_THAT(HandleMouseDownAndCountFires(
                  mouse_rocker_recognizer, fired_count_,
                  MakeMouseDown(blink::WebMouseEvent::Button::kLeft,
                                blink::WebInputEvent::kRightButtonDown)),
              testing::Pair(true, 1));
}

IN_PROC_BROWSER_TEST_F(
    GestureMouseRockerRecognizerBrowserTest,
    HandleMouseDownDoesNothingForAPlainClickWithNoOtherButtonHeld) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_THAT(HandleMouseDownAndCountFires(
                  mouse_rocker_recognizer, fired_count_,
                  MakeMouseDown(blink::WebMouseEvent::Button::kRight,
                                blink::WebInputEvent::kNoModifiers)),
              testing::Pair(false, 0));
}

IN_PROC_BROWSER_TEST_F(
    GestureMouseRockerRecognizerBrowserTest,
    HandleMouseDownDoesNothingWhenRockerGesturesAreDisabled) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetBoolean(
      prefs::kMouseRockerGesturesEnabled, false);
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer();

  EXPECT_THAT(HandleMouseDownAndCountFires(
                  mouse_rocker_recognizer, fired_count_,
                  MakeMouseDown(blink::WebMouseEvent::Button::kRight,
                                blink::WebInputEvent::kLeftButtonDown)),
              testing::Pair(false, 0));
}

IN_PROC_BROWSER_TEST_F(GestureMouseRockerRecognizerBrowserTest,
                       HandleMouseDownDoesNothingWhenNotAllowedForSite) {
  GestureMouseRockerRecognizer mouse_rocker_recognizer =
      MakeMouseRockerRecognizer(/*is_allowed_for_site=*/false);

  EXPECT_THAT(HandleMouseDownAndCountFires(
                  mouse_rocker_recognizer, fired_count_,
                  MakeMouseDown(blink::WebMouseEvent::Button::kRight,
                                blink::WebInputEvent::kLeftButtonDown)),
              testing::Pair(false, 0));
}

}  // namespace gestures
