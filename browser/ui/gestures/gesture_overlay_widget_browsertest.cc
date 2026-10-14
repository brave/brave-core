// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_overlay_widget.h"

#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"

namespace gestures {

namespace {

using GestureOverlayWidgetBrowserTest = PlatformBrowserTest;

}  // namespace

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       IsShownReturnsFalseUntilShowIsCalled) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));

  EXPECT_FALSE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       ShowMakesTheOverlayVisible) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));

  overlay_widget.Show();

  EXPECT_TRUE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       CallingShowTwiceKeepsOverlayVisible) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));
  overlay_widget.Show();

  overlay_widget.Show();

  EXPECT_TRUE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       HideMakesTheOverlayNotVisible) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));
  overlay_widget.Show();

  overlay_widget.Hide();

  EXPECT_FALSE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       ShowMakesOverlayVisibleAgainAfterHide) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));
  overlay_widget.Show();
  overlay_widget.Hide();

  overlay_widget.Show();

  EXPECT_TRUE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       UpdateTrailBeforeShowLeavesOverlayHidden) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));

  overlay_widget.UpdateTrail({gfx::PointF(0, 0), gfx::PointF(10, 10)},
                             /*show_trail=*/true, u"Back");

  EXPECT_FALSE(overlay_widget.IsShown());
}

IN_PROC_BROWSER_TEST_F(GestureOverlayWidgetBrowserTest,
                       UpdateTrailAfterShowKeepsOverlayVisible) {
  GestureOverlayWidget overlay_widget(
      chrome_test_utils::GetActiveWebContents(this));
  overlay_widget.Show();

  overlay_widget.UpdateTrail({gfx::PointF(0, 0), gfx::PointF(10, 10)},
                             /*show_trail=*/true, u"Back");

  EXPECT_TRUE(overlay_widget.IsShown());
}

}  // namespace gestures
