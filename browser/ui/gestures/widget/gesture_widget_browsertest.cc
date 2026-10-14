// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/widget/gesture_widget.h"

#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"

namespace gestures {

class GestureWidgetBrowserTest : public PlatformBrowserTest {
 protected:
  content::WebContents* active_web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest, ShouldRemainHiddenUntilShown) {
  GestureWidget gesture_widget(active_web_contents());

  EXPECT_FALSE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldShowOverlayWhenRequested) {
  GestureWidget gesture_widget(active_web_contents());

  gesture_widget.Show();

  EXPECT_TRUE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldKeepOverlayVisibleWhenShownTwice) {
  GestureWidget gesture_widget(active_web_contents());
  gesture_widget.Show();

  gesture_widget.Show();

  EXPECT_TRUE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldHideOverlayWhenRequested) {
  GestureWidget gesture_widget(active_web_contents());
  gesture_widget.Show();

  gesture_widget.Hide();

  EXPECT_FALSE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldShowOverlayAfterBeingHidden) {
  GestureWidget gesture_widget(active_web_contents());
  gesture_widget.Show();
  gesture_widget.Hide();

  gesture_widget.Show();

  EXPECT_TRUE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldKeepOverlayHiddenWhenTrailUpdatesBeforeShow) {
  GestureWidget gesture_widget(active_web_contents());

  gesture_widget.UpdateTrail(/*show_trail=*/true,
                             {gfx::PointF(0, 0), gfx::PointF(10, 10)}, u"Back");

  EXPECT_FALSE(gesture_widget.IsVisible());
}

IN_PROC_BROWSER_TEST_F(GestureWidgetBrowserTest,
                       ShouldKeepOverlayVisibleWhenTrailUpdatesAfterShow) {
  GestureWidget gesture_widget(active_web_contents());
  gesture_widget.Show();

  gesture_widget.UpdateTrail(/*show_trail=*/true,
                             {gfx::PointF(0, 0), gfx::PointF(10, 10)}, u"Back");

  EXPECT_TRUE(gesture_widget.IsVisible());
}

}  // namespace gestures
