// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_mouse_wheel_recognizer.h"

#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "brave/browser/ui/gestures/gesture_action_dispatcher.h"
#include "brave/browser/ui/gestures/gesture_config_tracker.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "ui/events/types/scroll_types.h"
#include "url/gurl.h"

namespace gestures {

namespace {

blink::WebMouseWheelEvent MakeWheelEvent(
    float delta_y,
    int modifiers,
    ui::ScrollGranularity delta_units = ui::ScrollGranularity::kScrollByPixel) {
  blink::WebMouseWheelEvent event(blink::WebInputEvent::Type::kMouseWheel,
                                  modifiers, base::TimeTicks::Now());
  event.delta_y = delta_y;
  event.delta_units = delta_units;
  return event;
}

}  // namespace

class GestureMouseWheelRecognizerBrowserTest : public PlatformBrowserTest {
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

  // The modifier-button-held/sensitivity logic under test is driven entirely
  // by prefs, so each test sets `prefs::kMouseWheelSensitivityInPixels`
  // directly before this first constructs `GestureConfigTracker`.
  GestureMouseWheelRecognizer MakeMouseWheelRecognizer() {
    return {config_tracker(), action_dispatcher(),
            base::BindRepeating(
                &GestureMouseWheelRecognizerBrowserTest::OnRightButtonFired,
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

  void OnRightButtonFired() { ++fired_count_; }

  size_t fired_count_ = 0;

 private:
  std::unique_ptr<GestureConfigTracker> config_tracker_;
  std::unique_ptr<GestureActionDispatcher> action_dispatcher_;
};

IN_PROC_BROWSER_TEST_F(GestureMouseWheelRecognizerBrowserTest,
                       HandleEventDoesNothingWhenModifierButtonIsNotHeld) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 10.0);
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/100, blink::WebInputEvent::kNoModifiers));

  EXPECT_EQ(0U, fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseWheelRecognizerBrowserTest,
                       HandleEventFiresWhenAccumulatedDeltaCrossesSensitivity) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 100.0);
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/30, blink::WebInputEvent::kRightButtonDown));
  EXPECT_EQ(0U, fired_count_);

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/30, blink::WebInputEvent::kRightButtonDown));
  EXPECT_EQ(0U, fired_count_);

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/50, blink::WebInputEvent::kRightButtonDown));
  EXPECT_EQ(1U, fired_count_);
}

IN_PROC_BROWSER_TEST_F(
    GestureMouseWheelRecognizerBrowserTest,
    HandleEventResetsTheAccumulatorWhenTheModifierButtonIsReleased) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 100.0);
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/80, blink::WebInputEvent::kRightButtonDown));
  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/0, blink::WebInputEvent::kNoModifiers));
  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/80, blink::WebInputEvent::kRightButtonDown));

  EXPECT_EQ(0U, fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseWheelRecognizerBrowserTest,
                       ResetClearsTheAccumulator) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 100.0);
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/80, blink::WebInputEvent::kRightButtonDown));
  mouse_wheel_recognizer.Reset();
  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/80, blink::WebInputEvent::kRightButtonDown));

  EXPECT_EQ(0U, fired_count_);
}

IN_PROC_BROWSER_TEST_F(GestureMouseWheelRecognizerBrowserTest,
                       HandleEventDispatchesThePreviousTabOnPositiveDelta) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 10.0);
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/true);
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  // `delta_y` runs positive in the up direction, which dispatches
  // `wheel_up_actions()`; its registered default is `kPreviousTab`.
  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/20, blink::WebInputEvent::kRightButtonDown));

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return browser()->tab_strip_model()->active_index() == 0; }));
}

IN_PROC_BROWSER_TEST_F(GestureMouseWheelRecognizerBrowserTest,
                       HandleEventDispatchesTheNextTabOnNegativeDelta) {
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 10.0);
  chrome::AddTabAt(browser(), GURL("about:blank"), /*index=*/-1,
                   /*foreground=*/false);
  ASSERT_EQ(0, browser()->tab_strip_model()->active_index());
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  // `delta_y` runs negative in the down direction, which dispatches
  // `wheel_down_actions()`; its registered default is `kNextTab`.
  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/-20, blink::WebInputEvent::kRightButtonDown));

  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return browser()->tab_strip_model()->active_index() == 1; }));
}

IN_PROC_BROWSER_TEST_F(
    GestureMouseWheelRecognizerBrowserTest,
    HandleEventCountsPageGranularityDeltaRegardlessOfSensitivity) {
  // A page-granularity delta (e.g. Windows' "scroll one screen at a time"
  // mouse setting) is already a single deliberate action, so it always
  // counts even though its magnitude is far below the pinned sensitivity.
  chrome_test_utils::GetProfile(this)->GetPrefs()->SetDouble(
      prefs::kMouseWheelSensitivityInPixels, 1000.0);
  GestureMouseWheelRecognizer mouse_wheel_recognizer =
      MakeMouseWheelRecognizer();

  mouse_wheel_recognizer.HandleEvent(
      MakeWheelEvent(/*delta_y=*/1, blink::WebInputEvent::kRightButtonDown,
                     ui::ScrollGranularity::kScrollByPage));

  EXPECT_EQ(1U, fired_count_);
}

}  // namespace gestures
